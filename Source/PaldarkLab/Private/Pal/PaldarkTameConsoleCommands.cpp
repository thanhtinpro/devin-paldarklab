// PALDARK W35-36 — Tame / Bond console commands.
//
// Four `FAutoConsoleCommandWithWorldAndArgs` registrations:
//
//   Paldark.Tame.Force <PalLabelOrName>
//     Finds a hostile `APaldarkPalCharacter` by partial name / label match
//     and force-tames it via `UPaldarkPalTameComponent::ForceTame`. Bypasses
//     the capture-probability formula so testers can iterate the roster
//     append flow without authoring `DA_PalDef_*` + Pal Sphere items.
//
//   Paldark.Tame.DumpRoster
//     Logs every entry in the local player's
//     `UPaldarkPlayerPalRosterComponent` (species, bond, health, definition
//     id, nickname). Server / standalone only — roster is server-mutated.
//
//   Paldark.Tame.SetStun <PalLabelOrName> <0..1>
//     Sets the Stun attribute on a Pal as a fraction of MaxStun. Useful for
//     driving the capture-probability formula's StunPct contribution
//     without authoring `GE_StunOnHit`. Clamps the fraction to [0, 1].
//
//   Paldark.Pal.Bond.AddXP <PalLabelOrName> <Amount> [ReasonTag]
//     Adds bond XP to a Pal's `UPaldarkPalBondComponent`. ReasonTag is
//     optional and defaults to `Paldark.Bond.Event.Tame`. Validates
//     ReasonTag is a real registered tag before forwarding.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 35-36.

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"

#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Pal/Components/PaldarkPalBondComponent.h"
#include "Pal/Components/PaldarkPalTameComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"

namespace PaldarkTameConsole
{
	static APaldarkPalCharacter* FindPalByLabel(UWorld* World, const FString& NeedleLower)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
		{
			APaldarkPalCharacter* Pal = *It;
			if (!Pal) { continue; }
			const FString Name = Pal->GetName().ToLower();
			const FString Label = Pal->GetActorLabel().ToLower();
			if (Name.Contains(NeedleLower) || Label.Contains(NeedleLower))
			{
				return Pal;
			}
		}
		return nullptr;
	}

	static APaldarkCharacter* GetLocalPaldarkPlayer(UWorld* World)
	{
		APlayerController* PC = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		return PC ? Cast<APaldarkCharacter>(PC->GetPawn()) : nullptr;
	}

	static void Force(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkPal, Warning, TEXT("Paldark.Tame.Force — no world."));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.Force — usage: Paldark.Tame.Force <PalLabelOrName>"));
			return;
		}

		const FString Needle = Args[0].ToLower();
		APaldarkPalCharacter* Pal = FindPalByLabel(World, Needle);
		if (!Pal)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.Force — no Pal matching '%s' in world."), *Needle);
			return;
		}
		if (!Pal->HasAuthority())
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.Force — Pal %s is not authoritative on this client."),
				*Pal->GetName());
			return;
		}

		UPaldarkPalTameComponent* Tame = Pal->GetTameSlot();
		if (!Tame)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.Force — Pal %s has no TameSlot."), *Pal->GetName());
			return;
		}

		APaldarkCharacter* Player = GetLocalPaldarkPlayer(World);
		if (!Player)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.Force — no local PaldarkCharacter pawn (run on listen/standalone server)."));
			return;
		}

		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Tame.Force — force-taming Pal %s for player %s."),
			*Pal->GetName(), *Player->GetName());
		Tame->ForceTame(Player);
	}

	static void DumpRoster(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkPal, Warning, TEXT("Paldark.Tame.DumpRoster — no world."));
			return;
		}
		APaldarkCharacter* Player = GetLocalPaldarkPlayer(World);
		if (!Player)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.DumpRoster — no local PaldarkCharacter pawn."));
			return;
		}
		UPaldarkPlayerPalRosterComponent* Roster = Player->GetRosterSlot();
		if (!Roster)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.DumpRoster — local player %s has no RosterSlot."),
				*Player->GetName());
			return;
		}
		Roster->DumpToLog();
	}

	static void SetStun(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkPal, Warning, TEXT("Paldark.Tame.SetStun — no world."));
			return;
		}
		if (Args.Num() < 2)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.SetStun — usage: Paldark.Tame.SetStun <PalLabelOrName> <0..1>"));
			return;
		}

		const FString Needle = Args[0].ToLower();
		APaldarkPalCharacter* Pal = FindPalByLabel(World, Needle);
		if (!Pal)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.SetStun — no Pal matching '%s' in world."), *Needle);
			return;
		}
		if (!Pal->HasAuthority())
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.SetStun — Pal %s is not authoritative on this client."),
				*Pal->GetName());
			return;
		}

		const float Fraction = FMath::Clamp(FCString::Atof(*Args[1]), 0.0f, 1.0f);

		UAbilitySystemComponent* ASC = Pal->GetAbilitySystemComponent();
		const UPaldarkAttributeSet* AttrSet = Pal->GetPaldarkAttributeSet();
		if (!ASC || !AttrSet)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Tame.SetStun — Pal %s missing ASC / AttributeSet."), *Pal->GetName());
			return;
		}
		const float MaxStun = AttrSet->GetMaxStun();
		const float NewStun = MaxStun * Fraction;
		ASC->SetNumericAttributeBase(UPaldarkAttributeSet::GetStunAttribute(), NewStun);
		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Tame.SetStun — Pal %s Stun=%.2f/%.2f (frac=%.2f)."),
			*Pal->GetName(), NewStun, MaxStun, Fraction);
	}

	static void BondAddXP(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkPal, Warning, TEXT("Paldark.Pal.Bond.AddXP — no world."));
			return;
		}
		if (Args.Num() < 2)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.Bond.AddXP — usage: Paldark.Pal.Bond.AddXP <PalLabelOrName> <Amount> [ReasonTag]"));
			return;
		}

		const FString Needle = Args[0].ToLower();
		APaldarkPalCharacter* Pal = FindPalByLabel(World, Needle);
		if (!Pal)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.Bond.AddXP — no Pal matching '%s' in world."), *Needle);
			return;
		}
		if (!Pal->HasAuthority())
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.Bond.AddXP — Pal %s is not authoritative on this client."),
				*Pal->GetName());
			return;
		}

		UPaldarkPalBondComponent* Bond = Pal->GetBondSlot();
		if (!Bond)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.Bond.AddXP — Pal %s has no BondSlot."), *Pal->GetName());
			return;
		}

		const float Amount = FCString::Atof(*Args[1]);

		FGameplayTag ReasonTag = PaldarkGameplayTags::TAG_Paldark_Bond_Event_Tame;
		if (Args.Num() >= 3)
		{
			ReasonTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Args[2]), /*ErrorIfNotFound*/ false);
			if (!ReasonTag.IsValid())
			{
				UE_LOG(LogPaldarkPal, Warning,
					TEXT("Paldark.Pal.Bond.AddXP — unknown ReasonTag '%s', falling back to Paldark.Bond.Event.Tame."),
					*Args[2]);
				ReasonTag = PaldarkGameplayTags::TAG_Paldark_Bond_Event_Tame;
			}
		}

		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Pal.Bond.AddXP — Pal %s +%.1f XP (Reason=%s)."),
			*Pal->GetName(), Amount, *ReasonTag.ToString());
		Bond->AddBondXP(Amount, ReasonTag);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GPaldarkTameForceCmd(
	TEXT("Paldark.Tame.Force"),
	TEXT("(W35-36) Force-tame a hostile Pal (bypass capture-probability formula). Args: <PalLabelOrName>."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkTameConsole::Force));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkTameDumpRosterCmd(
	TEXT("Paldark.Tame.DumpRoster"),
	TEXT("(W35-36) Dump the local player's tamed-Pal roster."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkTameConsole::DumpRoster));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkTameSetStunCmd(
	TEXT("Paldark.Tame.SetStun"),
	TEXT("(W35-36) Set a Pal's Stun attribute as a fraction of MaxStun. Args: <PalLabelOrName> <0..1>."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkTameConsole::SetStun));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkBondAddXPCmd(
	TEXT("Paldark.Pal.Bond.AddXP"),
	TEXT("(W35-36) Grant bond XP to a Pal. Args: <PalLabelOrName> <Amount> [ReasonTag=Paldark.Bond.Event.Tame]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkTameConsole::BondAddXP));
