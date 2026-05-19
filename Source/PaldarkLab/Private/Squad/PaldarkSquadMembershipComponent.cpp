// PALDARK W22-23 — Squad membership component implementation.

#include "Squad/PaldarkSquadMembershipComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Squad/PaldarkSquadSubsystem.h"

UPaldarkSquadMembershipComponent::UPaldarkSquadMembershipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
	SquadTag = PaldarkGameplayTags::TAG_Paldark_Squad_Default;
}

void UPaldarkSquadMembershipComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		return;
	}

	UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem();
	APaldarkCharacter* PlayerChar = GetOwnerCharacter();
	if (SquadSub == nullptr || PlayerChar == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[SquadMembership] BeginPlay on %s — subsystem=%p char=%p, skipping."),
			*Owner->GetName(), SquadSub, PlayerChar);
		return;
	}

	RegisteredTag = SquadTag.IsValid() ? SquadTag : PaldarkGameplayTags::TAG_Paldark_Squad_Default;
	SquadSub->RegisterPlayerToSquad(PlayerChar, RegisteredTag);
}

void UPaldarkSquadMembershipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AActor* Owner = GetOwner();
	if (Owner != nullptr && Owner->HasAuthority())
	{
		if (UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem())
		{
			if (APaldarkCharacter* PlayerChar = GetOwnerCharacter())
			{
				SquadSub->UnregisterPlayerFromSquad(PlayerChar);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UPaldarkSquadMembershipComponent::SetSquadTag(const FGameplayTag& InSquadTag)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[SquadMembership] SetSquadTag on non-authority — rejected."));
		return;
	}

	const FGameplayTag NewTag = InSquadTag.IsValid()
		? InSquadTag
		: PaldarkGameplayTags::TAG_Paldark_Squad_Default;

	if (NewTag == RegisteredTag)
	{
		return;
	}

	UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem();
	APaldarkCharacter* PlayerChar = GetOwnerCharacter();
	if (SquadSub == nullptr || PlayerChar == nullptr)
	{
		return;
	}

	if (RegisteredTag.IsValid())
	{
		SquadSub->UnregisterPlayerFromSquad(PlayerChar);
	}

	SquadTag = NewTag;
	RegisteredTag = NewTag;
	SquadSub->RegisterPlayerToSquad(PlayerChar, RegisteredTag);
}

UPaldarkSquadSubsystem* UPaldarkSquadMembershipComponent::GetSquadSubsystem() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UPaldarkSquadSubsystem>() : nullptr;
}

APaldarkCharacter* UPaldarkSquadMembershipComponent::GetOwnerCharacter() const
{
	return Cast<APaldarkCharacter>(GetOwner());
}
