#include "Pal/PaldarkHostilePackSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

UPaldarkHostilePackSubsystem::UPaldarkHostilePackSubsystem()
{
	// Defaults set in header. Constructor body kept so future weeks can
	// hook into the lifecycle without touching the header.
}

bool UPaldarkHostilePackSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Game-world only — skip editor preview worlds and inactive maps so the
	// registry doesn't accumulate ghost packs across PIE sessions. The
	// matching server-only filter happens at Register/Broadcast time since
	// the subsystem is created on the client too but the registry stays
	// empty (no Pal calls Register from a client).
	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}
	const EWorldType::Type WorldType = World->WorldType;
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE;
}

void UPaldarkHostilePackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkHostilePackSubsystem::Initialize — pack broadcast radius=%.1f cm"),
		PackBroadcastRadius);
}

void UPaldarkHostilePackSubsystem::Deinitialize()
{
	PackRegistry.Reset();
	Super::Deinitialize();
}

void UPaldarkHostilePackSubsystem::RegisterPalToPack(APaldarkPalCharacter* InPal, const FGameplayTag& InPackTag)
{
	if (InPal == nullptr || !InPackTag.IsValid())
	{
		return;
	}

	// Server-only — clients never call this in normal flow, but guard
	// defensively so a future C++ subclass calling Register at the wrong
	// time doesn't corrupt the registry.
	if (!InPal->HasAuthority())
	{
		return;
	}

	FPaldarkHostilePack& Pack = PackRegistry.FindOrAdd(InPackTag);
	const TWeakObjectPtr<APaldarkPalCharacter> WeakPal = InPal;
	if (!Pack.Members.Contains(WeakPal))
	{
		Pack.Members.Add(WeakPal);
		UE_LOG(LogPaldarkPal, Log,
			TEXT("UPaldarkHostilePackSubsystem::RegisterPalToPack — pack=%s pal=%s size=%d"),
			*InPackTag.ToString(),
			*InPal->GetName(),
			Pack.Members.Num());
	}
}

void UPaldarkHostilePackSubsystem::UnregisterPalFromPack(APaldarkPalCharacter* InPal)
{
	if (InPal == nullptr)
	{
		return;
	}
	const TWeakObjectPtr<APaldarkPalCharacter> WeakPal = InPal;
	for (TPair<FGameplayTag, FPaldarkHostilePack>& Entry : PackRegistry)
	{
		const int32 Removed = Entry.Value.Members.Remove(WeakPal);
		if (Removed > 0)
		{
			UE_LOG(LogPaldarkPal, Log,
				TEXT("UPaldarkHostilePackSubsystem::UnregisterPalFromPack — pack=%s pal=%s remaining=%d"),
				*Entry.Key.ToString(),
				*InPal->GetName(),
				Entry.Value.Members.Num());
		}
	}
}

void UPaldarkHostilePackSubsystem::BroadcastPackThreat(APaldarkPalCharacter* Initiator, AActor* NewThreat)
{
	if (Initiator == nullptr || NewThreat == nullptr || !IsValid(NewThreat))
	{
		return;
	}
	if (!Initiator->HasAuthority())
	{
		return;
	}

	const FGameplayTag PackTag = Initiator->GetPackTag();
	if (!PackTag.IsValid())
	{
		// Lone hostile Pal — no pack to broadcast to. Common for Razorbird
		// (the W20-21 deliverable is "1 Razorbird" so the spawner leaves
		// the pack tag empty in that flow).
		return;
	}

	const FPaldarkHostilePack* Pack = PackRegistry.Find(PackTag);
	if (Pack == nullptr || Pack->Members.Num() == 0)
	{
		return;
	}

	const FVector InitiatorLocation = Initiator->GetActorLocation();
	const float   RadiusSq          = PackBroadcastRadius * PackBroadcastRadius;

	int32 NotifiedCount = 0;
	for (const TWeakObjectPtr<APaldarkPalCharacter>& Weak : Pack->Members)
	{
		APaldarkPalCharacter* Packmate = Weak.Get();
		if (Packmate == nullptr || Packmate == Initiator || !IsValid(Packmate))
		{
			continue;
		}

		// Distance gate — far-away packmates don't aggro on a single
		// member's sighting (keeps the pack feel "local").
		if (FVector::DistSquared(Packmate->GetActorLocation(), InitiatorLocation) > RadiusSq)
		{
			continue;
		}

		UPaldarkPalPerceptionComponent* Perception = Packmate->GetPerceptionSlot();
		if (Perception == nullptr)
		{
			continue;
		}

		// Feedback-loop guard — if Packmate already considers NewThreat its
		// current threat, ForceThreat would re-fire OnThreatChanged with
		// the same value and we'd recurse forever. Skip the call instead.
		if (Perception->GetCurrentThreat() == NewThreat)
		{
			continue;
		}

		Perception->ForceThreat(NewThreat);
		++NotifiedCount;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkHostilePackSubsystem::BroadcastPackThreat — pack=%s initiator=%s target=%s notified=%d/%d"),
		*PackTag.ToString(),
		*Initiator->GetName(),
		*NewThreat->GetName(),
		NotifiedCount,
		Pack->Members.Num());
}

int32 UPaldarkHostilePackSubsystem::GetPackSize(const FGameplayTag& InPackTag) const
{
	const FPaldarkHostilePack* Pack = PackRegistry.Find(InPackTag);
	return Pack != nullptr ? Pack->Members.Num() : 0;
}

void UPaldarkHostilePackSubsystem::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkHostilePackSubsystem::DumpToLog — %d pack tag(s) registered, broadcast_radius=%.1f"),
		PackRegistry.Num(),
		PackBroadcastRadius);
	for (const TPair<FGameplayTag, FPaldarkHostilePack>& Entry : PackRegistry)
	{
		UE_LOG(LogPaldarkPal, Log,
			TEXT("  pack=%s members=%d"),
			*Entry.Key.ToString(),
			Entry.Value.Members.Num());
		for (const TWeakObjectPtr<APaldarkPalCharacter>& Weak : Entry.Value.Members)
		{
			const APaldarkPalCharacter* Pal = Weak.Get();
			UE_LOG(LogPaldarkPal, Log,
				TEXT("    member=%s location=%s species=%s"),
				Pal != nullptr ? *Pal->GetName() : TEXT("<gc>"),
				Pal != nullptr ? *Pal->GetActorLocation().ToCompactString() : TEXT("?"),
				Pal != nullptr ? *Pal->GetSpeciesTag().ToString() : TEXT("?"));
		}
	}
}
