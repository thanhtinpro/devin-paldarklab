#include "Pal/Activities/PaldarkActivity_Combat.h"

#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

#include "Gas/PaldarkAttributeSet.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkActivity_Combat::UPaldarkActivity_Combat()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Combat;
	// Priority 40 — higher than Investigate (30) so a sudden threat preempts
	// an in-progress ping investigation.
	Priority    = 40;
}

bool UPaldarkActivity_Combat::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (!Super::InitActivity(InComponent))
	{
		return false;
	}

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		CombatRef     = Pal->GetCombatSlot();
		PerceptionRef = Pal->GetPerceptionSlot();
	}

	if (!CombatRef.IsValid() || !PerceptionRef.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkActivity_Combat::InitActivity — missing combat/perception slot on pal %s (combat=%d perception=%d). Activity will never run."),
			*GetNameSafe(GetPalOwner()),
			CombatRef.IsValid() ? 1 : 0,
			PerceptionRef.IsValid() ? 1 : 0);
	}
	return true;
}

bool UPaldarkActivity_Combat::BuildConsiderations(TArray<FPaldarkConsideration>& OutConsiderations) const
{
	UPaldarkPalCombatComponent*     Combat     = CombatRef.Get();
	UPaldarkPalPerceptionComponent* Perception = PerceptionRef.Get();
	APaldarkPalCharacter*           Pal        = GetPalOwner();
	if (Combat == nullptr || Perception == nullptr || Pal == nullptr)
	{
		return false;
	}

	// 1) ThreatDistance — closer = scarier. 1 - clamp(D / MaxRange, 0, 1).
	const float MaxRange = FMath::Max(Combat->MaxEngageRange, 1.f);
	const float Distance = Perception->GetCurrentThreatDistance();
	const float ThreatDistanceScore = 1.f - FMath::Clamp(Distance / MaxRange, 0.f, 1.f);

	// 2) PalHealth — full health = 1, empty = 0. A wounded Pal still wants
	// to fight (we don't gate at 0); designers can flip the weight if they
	// want flee behaviour to dominate.
	float HealthScore = 1.f;
	if (const UPaldarkAttributeSet* Attr = Pal->GetPaldarkAttributeSet())
	{
		const float Max = FMath::Max(Attr->GetMaxHealth(), 1.f);
		HealthScore = FMath::Clamp(Attr->GetHealth() / Max, 0.f, 1.f);
	}

	// 3) AttackReady — 1 when off cooldown, 0 when on cooldown. The score
	// drops to 0 for a heartbeat right after each fire so very-low-priority
	// considerations can swap in mid-fight if designers want (e.g. flee
	// when stamina exhausted, future W22+).
	const float AttackReadyScore = Combat->IsAttackReady() ? 1.f : 0.f;

	OutConsiderations.Reset();
	OutConsiderations.Add({TEXT("ThreatDistance"), ThreatDistanceScore, ThreatDistanceWeight});
	OutConsiderations.Add({TEXT("PalHealth"),      HealthScore,         PalHealthWeight});
	OutConsiderations.Add({TEXT("AttackReady"),    AttackReadyScore,    AttackReadyWeight});
	return true;
}

bool UPaldarkActivity_Combat::CanRun_Implementation() const
{
	UPaldarkPalCombatComponent*     Combat     = CombatRef.Get();
	UPaldarkPalPerceptionComponent* Perception = PerceptionRef.Get();
	if (Combat == nullptr || Perception == nullptr)
	{
		return false;
	}

	// First filter — no target, no combat. The utility score below still
	// runs for visibility (DumpThreat surfaces it) but only when the target
	// pointer is live.
	if (!Perception->HasThreat() || Combat->GetCurrentTarget() == nullptr)
	{
		LastUtilityScore = 0.f;
		return false;
	}

	TArray<FPaldarkConsideration> Considerations;
	if (!BuildConsiderations(Considerations))
	{
		LastUtilityScore = 0.f;
		return false;
	}

	LastUtilityScore = PaldarkPalConsiderations::ComputeUtilityScore(Considerations);
	return LastUtilityScore >= MinUtilityScore;
}

bool UPaldarkActivity_Combat::ShouldContinue_Implementation() const
{
	UPaldarkPalCombatComponent* Combat = CombatRef.Get();
	if (Combat == nullptr)
	{
		return false;
	}

	AActor* Target = Combat->GetCurrentTarget();
	if (Target == nullptr || !IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		return false;
	}

	const APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal == nullptr)
	{
		return false;
	}

	const float Distance = FVector::Dist(Target->GetActorLocation(), Pal->GetActorLocation());
	return Distance <= DisengageDistance;
}

void UPaldarkActivity_Combat::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();

	// Pause the follow leash while we fight. Locomotion remembers the
	// FollowedPawn cache so Exit re-enables seamlessly.
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(false);
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkActivity_Combat::EnterActivity — pal=%s entering combat (score=%.2f, target=%s)"),
		*GetNameSafe(GetPalOwner()),
		LastUtilityScore,
		CombatRef.IsValid() ? *GetNameSafe(CombatRef->GetCurrentTarget()) : TEXT("<null>"));
}

void UPaldarkActivity_Combat::TickActivity_Implementation(float DeltaSeconds)
{
	UPaldarkPalCombatComponent* Combat = CombatRef.Get();
	APaldarkPalCharacter*       Pal    = GetPalOwner();
	if (Combat == nullptr || Pal == nullptr)
	{
		return;
	}

	AActor* Target = Combat->GetCurrentTarget();
	if (Target != nullptr && IsValid(Target))
	{
		// Re-orient toward target. The Pal pawn uses controller-desired
		// rotation NOT orient-to-movement (see APaldarkPalCharacter ctor),
		// so SetActorRotation is safe here. Lerp via FaceTargetRateDegPerSec
		// so the rotation reads smoothly when montages land in W22+.
		const FVector  ToTarget    = (Target->GetActorLocation() - Pal->GetActorLocation()).GetSafeNormal2D();
		const FRotator CurrentRot  = Pal->GetActorRotation();
		const FRotator DesiredYaw  = FRotator(0.f, ToTarget.Rotation().Yaw, 0.f);
		const FRotator BlendedRot  = FMath::RInterpConstantTo(CurrentRot, DesiredYaw, DeltaSeconds, FaceTargetRateDegPerSec);
		Pal->SetActorRotation(FRotator(CurrentRot.Pitch, BlendedRot.Yaw, CurrentRot.Roll));
	}

	// Tip the combat component every tick — it gates internally on cooldown
	// + range so this is cheap when the Pal is "waiting" between attacks.
	Combat->TryFireAttack();
}

void UPaldarkActivity_Combat::ExitActivity_Implementation()
{
	Super::ExitActivity_Implementation();

	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(true);
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkActivity_Combat::ExitActivity — pal=%s leaving combat (last_score=%.2f)"),
		*GetNameSafe(GetPalOwner()),
		LastUtilityScore);
}
