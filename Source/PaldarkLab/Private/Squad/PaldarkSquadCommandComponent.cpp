// PALDARK W22-23 — Squad command component implementation.

#include "Squad/PaldarkSquadCommandComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Squad/PaldarkSquadCommandSet.h"
#include "Squad/PaldarkSquadSubsystem.h"

UPaldarkSquadCommandComponent::UPaldarkSquadCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPaldarkSquadCommandComponent::IssueCommand(
	FGameplayTag InCommandTag,
	FVector OptionalLocation)
{
	if (!InCommandTag.IsValid())
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[SquadCommand] IssueCommand: invalid tag — ignored."));
		return;
	}

	// Local validation — reject tags not in the designer-authored set so a
	// typo'd console command doesn't reach the server.
	if (const UPaldarkSquadCommandSet* Set = GetCommandSet())
	{
		FPaldarkSquadCommandRow Row;
		if (!Set->FindRowByTag(InCommandTag, Row))
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("[SquadCommand] IssueCommand: tag %s not in command set %s."),
				*InCommandTag.ToString(), *Set->GetName());
			return;
		}
	}
	// If no command set assigned, allow through — the headless test path
	// (no UI asset authored yet) uses console commands that don't need
	// designer validation. Server still re-validates squad membership.

	Server_IssueCommand(InCommandTag, OptionalLocation);
}

void UPaldarkSquadCommandComponent::Server_IssueCommand_Implementation(
	FGameplayTag InCommandTag,
	FVector OptionalLocation)
{
	APaldarkCharacter* Issuer = GetOwnerCharacter();
	UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem();
	if (Issuer == nullptr || SquadSub == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[SquadCommand] Server_IssueCommand: issuer=%p subsystem=%p — dropped."),
			Issuer, SquadSub);
		return;
	}

	SquadSub->BroadcastSquadCommand(Issuer, InCommandTag, OptionalLocation);
}

const UPaldarkSquadCommandSet* UPaldarkSquadCommandComponent::GetCommandSet() const
{
	if (CachedCommandSet != nullptr)
	{
		return CachedCommandSet;
	}

	if (CommandSetSoft.IsNull())
	{
		return nullptr;
	}

	// Synchronous load — radial wheel + command set are tiny assets and
	// only resolved on the first IssueCommand call. Async polish lands
	// W26+ with the rest of the AssetManager pass.
	CachedCommandSet = CommandSetSoft.LoadSynchronous();
	return CachedCommandSet;
}

APaldarkCharacter* UPaldarkSquadCommandComponent::GetOwnerCharacter() const
{
	return Cast<APaldarkCharacter>(GetOwner());
}

UPaldarkSquadSubsystem* UPaldarkSquadCommandComponent::GetSquadSubsystem() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UPaldarkSquadSubsystem>() : nullptr;
}
