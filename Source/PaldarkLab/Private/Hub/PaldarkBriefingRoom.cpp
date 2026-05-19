// PALDARK W40-41 — Briefing room hub building.

#include "Hub/PaldarkBriefingRoom.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Hub/PaldarkBriefingController.h"
#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkHubSubsystem.h"
#include "Map/PaldarkMapDefinition.h"
#include "Net/PaldarkNetSubsystem.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"
#include "TimerManager.h"

APaldarkBriefingRoom::APaldarkBriefingRoom()
{
	BuildingTag = PaldarkGameplayTags::TAG_Paldark_Hub_Building_BriefingRoom;
}

void APaldarkBriefingRoom::OnInteract(APaldarkPlayerController* Interactor)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// W46 — Primary path: route through the interactor's BriefingController
	// so the UMG widget (designer-authored `WBP_PaldarkBriefingWidget`) mounts
	// and the player joins the shard-wide vote/ready session. The actual
	// ServerTravel is issued by `UPaldarkBriefingSessionComponent` once the
	// vote+ready threshold is met (or by `Paldark.Hub.QA.ForceTravel`).
	if (Interactor != nullptr)
	{
		if (UPaldarkBriefingController* Controller = Interactor->GetBriefingController())
		{
			UE_LOG(LogPaldarkLab, Log,
				TEXT("[Hub.BriefingRoom %s] OnInteract by %s — opening briefing widget via BriefingController."),
				*GetDebugLabel(),
				*Interactor->GetName());
			Controller->RequestOpenBriefing(this);
			return;
		}
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Hub.BriefingRoom %s] OnInteract by %s — BriefingController missing; falling back to W40-41 console path."),
			*GetDebugLabel(),
			*Interactor->GetName());
	}

	// W40-41 fallback — solo / console-driven path. Retained so the
	// `Paldark.Hub.QueueRaid <MapTag>` smoke test continues to work even when
	// the briefing controller is not available (dedicated-server boot, AI
	// pawn standing in for a player, etc.).
	UPaldarkHubSubsystem* Hub = UPaldarkHubSubsystem::Get(World);
	if (Hub == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Hub.BriefingRoom %s] OnInteract — no hub subsystem; cannot resolve PendingRaidMapTag."),
			*GetDebugLabel());
		return;
	}

	const FGameplayTag PendingTag = Hub->GetPendingRaidMapTag();
	if (!PendingTag.IsValid())
	{
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[Hub.BriefingRoom %s] OnInteract by %s — no raid queued. Run `Paldark.Hub.QueueRaid <MapTag>` first."),
			*GetDebugLabel(),
			Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"));
		return;
	}

	// Validate against AllowedRaidMaps (if the definition lists any). Empty
	// allow-list means "any registered map" — designer test maps.
	FString ResolvedMapName;
	if (const UPaldarkHubBuildingDefinition* Def = BuildingDefinition.Get())
	{
		bool bFoundMatch = Def->AllowedRaidMaps.Num() == 0;
		for (const TSoftObjectPtr<UPaldarkMapDefinition>& SoftMap : Def->AllowedRaidMaps)
		{
			if (SoftMap.IsNull())
			{
				continue;
			}
			// Synchronous load — map defs are tiny config-only assets and
			// the briefing room is interacted off the hot path. The map's
			// MapTag is what we validate against the pending tag.
			UPaldarkMapDefinition* MapDef = SoftMap.LoadSynchronous();
			if (MapDef == nullptr)
			{
				continue;
			}
			if (MapDef->MapTag == PendingTag)
			{
				bFoundMatch = true;
				// MapAsset short-name → ServerTravel URL. Soft object path
				// like "/Game/Paldark/Maps/Map_RungHong.Map_RungHong" → "Map_RungHong".
				const FString AssetPath = MapDef->MapAsset.ToSoftObjectPath().GetAssetPathString();
				FString PackagePath;
				FString AssetName;
				if (!AssetPath.Split(TEXT("."), &PackagePath, &AssetName))
				{
					PackagePath = AssetPath;
				}
				ResolvedMapName = FPaths::GetCleanFilename(PackagePath);
				break;
			}
		}

		if (!bFoundMatch)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("[Hub.BriefingRoom %s] OnInteract by %s — pending raid tag %s not in AllowedRaidMaps (%d entries). Rejecting handoff."),
				*GetDebugLabel(),
				Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"),
				*PendingTag.ToString(),
				Def->AllowedRaidMaps.Num());
			return;
		}
	}

	// Fallback: if no definition / no resolved map name, ServerTravel using
	// the tag leaf as the map short name. Convention: `Paldark.Map.RungHong`
	// → `Map_RungHong`. Designer maps should always populate the def, so this
	// is only the test-map path.
	if (ResolvedMapName.IsEmpty())
	{
		const FString TagStr = PendingTag.ToString();
		int32 LastDot = INDEX_NONE;
		if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot < TagStr.Len() - 1)
		{
			ResolvedMapName = FString::Printf(TEXT("Map_%s"), *TagStr.RightChop(LastDot + 1));
		}
	}

	if (ResolvedMapName.IsEmpty())
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Hub.BriefingRoom %s] OnInteract — could not resolve a map name from tag %s. Aborting travel."),
			*GetDebugLabel(),
			*PendingTag.ToString());
		return;
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Hub.BriefingRoom %s] OnInteract by %s — queuing deferred ServerTravel to Map=%s (tag=%s)."),
		*GetDebugLabel(),
		Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"),
		*ResolvedMapName,
		*PendingTag.ToString());

	// Defer to next tick — see header comment. The lambda captures by value
	// since the strings live past the IssueDeferredTravel callsite.
	FTimerHandle Handle;
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &APaldarkBriefingRoom::IssueDeferredTravel,
			ResolvedMapName, FString()));
}

void APaldarkBriefingRoom::IssueDeferredTravel(FString MapName, FString ExperienceId)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	UGameInstance* GI = World->GetGameInstance();
	if (GI == nullptr)
	{
		return;
	}
	UPaldarkNetSubsystem* Net = GI->GetSubsystem<UPaldarkNetSubsystem>();
	if (Net == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Hub.BriefingRoom %s] IssueDeferredTravel — UPaldarkNetSubsystem not available; aborting."),
			*GetDebugLabel());
		return;
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Hub.BriefingRoom %s] IssueDeferredTravel — HostHubServer(%s, %s)."),
		*GetDebugLabel(),
		*MapName,
		ExperienceId.IsEmpty() ? TEXT("<none>") : *ExperienceId);
	Net->HostHubServer(MapName, ExperienceId);
}
