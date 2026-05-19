#include "Player/PaldarkCharacter.h"

#include "AbilitySystemComponent.h"
#include "Anim/PaldarkAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Combat/PaldarkLagCompensationComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

#include "Experience/PaldarkInputConfig.h"
#include "Experience/PaldarkPawnData.h"
#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Loot/PaldarkLootBag.h"
#include "Match/PaldarkMatchSubsystem.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerState.h"
#include "Player/Components/PaldarkPlayerActivityComponent.h"
#include "Player/Components/PaldarkPlayerCameraExtensionComponent.h"
#include "Player/Components/PaldarkPlayerCombatComponent.h"
#include "Player/Components/PaldarkPlayerDamageComponent.h"
#include "Player/Components/PaldarkPlayerEquipmentComponent.h"
#include "Player/Components/PaldarkPlayerHealthComponent.h"
#include "Player/Components/PaldarkPlayerInteractionComponent.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/Components/PaldarkPlayerLocomotionExtComponent.h"
#include "Player/Components/PaldarkPlayerNetworkComponent.h"
#include "Player/Components/PaldarkPlayerPalCompanionComponent.h"
#include "Player/Components/PaldarkPlayerStaminaComponent.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "Squad/PaldarkSquadCommandComponent.h"
#include "Squad/PaldarkSquadMembershipComponent.h"

APaldarkCharacter::APaldarkCharacter()
{
	// Yaw follows controller, pitch/roll handled by camera boom.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bConstrainToPlane = false;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(42.0f, 96.0f);
	}

	// Composition: SpringArm -> Camera. Designers can re-parent in Blueprint.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 80.0f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// W3-4 — 12 empty player component slots. Slot names match the field names
	// in the header so blueprint subclasses can swap implementations without
	// breaking serialization.
	HealthSlot         = CreateDefaultSubobject<UPaldarkPlayerHealthComponent>           (TEXT("HealthSlot"));
	StaminaSlot        = CreateDefaultSubobject<UPaldarkPlayerStaminaComponent>          (TEXT("StaminaSlot"));
	CombatSlot         = CreateDefaultSubobject<UPaldarkPlayerCombatComponent>           (TEXT("CombatSlot"));
	InventorySlot      = CreateDefaultSubobject<UPaldarkPlayerInventoryComponent>        (TEXT("InventorySlot"));
	EquipmentSlot      = CreateDefaultSubobject<UPaldarkPlayerEquipmentComponent>        (TEXT("EquipmentSlot"));
	PalCompanionSlot   = CreateDefaultSubobject<UPaldarkPlayerPalCompanionComponent>     (TEXT("PalCompanionSlot"));
	LocomotionExtSlot  = CreateDefaultSubobject<UPaldarkPlayerLocomotionExtComponent>    (TEXT("LocomotionExtSlot"));
	ActivitySlot       = CreateDefaultSubobject<UPaldarkPlayerActivityComponent>         (TEXT("ActivitySlot"));
	InteractionSlot    = CreateDefaultSubobject<UPaldarkPlayerInteractionComponent>      (TEXT("InteractionSlot"));
	CameraExtSlot      = CreateDefaultSubobject<UPaldarkPlayerCameraExtensionComponent>  (TEXT("CameraExtSlot"));
	NetworkSlot        = CreateDefaultSubobject<UPaldarkPlayerNetworkComponent>          (TEXT("NetworkSlot"));
	DamageSlot         = CreateDefaultSubobject<UPaldarkPlayerDamageComponent>           (TEXT("DamageSlot"));

	// W16-17 — Lag compensation slot. Always constructed (server creates +
	// ticks, clients hold the component as RPC routing target). The
	// component's BeginPlay disables tick on clients.
	LagCompSlot        = CreateDefaultSubobject<UPaldarkLagCompensationComponent>        (TEXT("LagCompSlot"));

	// W22-23 — Squad system slots. The membership component registers the
	// player into the squad subsystem on BeginPlay (authority only); the
	// command component owns the per-player Server_IssueCommand RPC for the
	// radial wheel.
	SquadMembershipSlot = CreateDefaultSubobject<UPaldarkSquadMembershipComponent>(TEXT("SquadMembershipSlot"));
	SquadCommandSlot    = CreateDefaultSubobject<UPaldarkSquadCommandComponent>   (TEXT("SquadCommandSlot"));

	// W35-36 — Per-player roster of tamed Pals. The component is
	// transient + match-scoped (save-game is W47); always constructed so
	// `UPaldarkPalTameComponent::RegisterAndDestroy` can `FindComponentByClass`
	// + call TamePal without per-controller subclass switching.
	RosterSlot          = CreateDefaultSubobject<UPaldarkPlayerPalRosterComponent>(TEXT("RosterSlot"));

	// W16-17 — Per-bone hitbox boxes. One UBoxComponent per skeletal mesh
	// socket; the lag compensation pipeline snapshots their transforms each
	// server tick and re-positions them during rewind. Default to
	// NoCollision so they don't interfere with live combat / movement —
	// EnableCharacterMeshCollision flips them to QueryOnly during the
	// rewind trace only.
	//
	// Bone-name list mirrors [10] Udemy MP Shooter (Blaster) + the default
	// UE5 mannequin (Manny / Quinn) skeleton. Designer can re-skin via
	// subclass + USkeletalMeshComponent override; the lag comp lookup is
	// by FName so the rewind code keeps working as long as the box names
	// match the new skeleton's socket names.
	//
	// Box extent defaults are loose — they cover the visible silhouette of
	// the UE5 mannequin. Designer can tighten them per-bone via the
	// component panel in BP_PaldarkCharacter once visual debug is on.
	struct FBoxSpec { const TCHAR* SocketName; FVector Extent; };
	static const FBoxSpec BoxSpecs[] =
	{
		{ TEXT("head"),         FVector(8.f,  8.f,  8.f) },
		{ TEXT("pelvis"),       FVector(17.f, 12.f, 20.f) },
		{ TEXT("spine_02"),     FVector(17.f, 13.f, 17.f) },
		{ TEXT("spine_03"),     FVector(17.f, 14.f, 13.f) },
		{ TEXT("upperarm_l"),   FVector(15.f,  6.f,  6.f) },
		{ TEXT("upperarm_r"),   FVector(15.f,  6.f,  6.f) },
		{ TEXT("lowerarm_l"),   FVector(14.f,  5.f,  5.f) },
		{ TEXT("lowerarm_r"),   FVector(14.f,  5.f,  5.f) },
		{ TEXT("hand_l"),       FVector( 8.f,  4.f,  4.f) },
		{ TEXT("hand_r"),       FVector( 8.f,  4.f,  4.f) },
		{ TEXT("thigh_l"),      FVector(20.f,  8.f,  8.f) },
		{ TEXT("thigh_r"),      FVector(20.f,  8.f,  8.f) },
		{ TEXT("calf_l"),       FVector(20.f,  7.f,  7.f) },
		{ TEXT("calf_r"),       FVector(20.f,  7.f,  7.f) },
		{ TEXT("foot_l"),       FVector(10.f,  5.f,  5.f) },
		{ TEXT("foot_r"),       FVector(10.f,  5.f,  5.f) },
	};

	// W18-19 — Team default. Pal perception treats this as friendly so a
	// Pal travelling with the player ignores them as a threat. Designer
	// flips per-Blueprint subclass for PvP / multi-team modes.
	TeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Player;

	// W37-38 — Drop-on-death default. Designer can override per-Blueprint
	// (e.g. PvE bots set this to null to suppress the bag). When the match
	// death hook fires, the inventory is moved into the spawned bag and
	// then cleared.
	PlayerDeathBagClass = APaldarkLootBag::StaticClass();

	USkeletalMeshComponent* MeshComp = GetMesh();
	for (const FBoxSpec& Spec : BoxSpecs)
	{
		const FString BoxName = FString::Printf(TEXT("HitBox_%s"), Spec.SocketName);
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(FName(*BoxName));
		if (Box != nullptr)
		{
			if (MeshComp != nullptr)
			{
				Box->SetupAttachment(MeshComp, FName(Spec.SocketName));
			}
			Box->SetBoxExtent(Spec.Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Box->SetCollisionObjectType(ECC_WorldDynamic);
			Box->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			HitCollisionBoxes.Emplace(FName(Spec.SocketName), Box);
		}
	}
}

void APaldarkCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogPaldark, Log, TEXT("APaldarkCharacter::BeginPlay — %s ready."), *GetName());
}

void APaldarkCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("APaldarkCharacter::PossessedBy — %s by %s"),
		*GetName(),
		NewController != nullptr ? *NewController->GetName() : TEXT("<null>"));

	// W7-8 — Server path. PlayerState is already created + replicated by the
	// time PossessedBy runs, so the ASC is reachable here.
	InitAbilitySystem();
}

void APaldarkCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// W7-8 — Autonomous proxy path. PlayerState replicates separately from
	// the pawn, so we wait until it lands here to resolve the ASC + bind
	// ability inputs.
	InitAbilitySystem();
}

void APaldarkCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("APaldarkCharacter::OnRep_Controller — %s sees new controller %s"),
		*GetName(),
		GetController() != nullptr ? *GetController()->GetName() : TEXT("<null>"));
}

void APaldarkCharacter::InitAbilitySystem()
{
	if (CachedAbilitySystemComponent != nullptr)
	{
		// Already resolved; nothing to do.
		return;
	}
	APaldarkPlayerState* PS = GetPlayerState<APaldarkPlayerState>();
	if (PS == nullptr)
	{
		return;
	}
	UPaldarkAbilitySystemComponent* ASC = PS->GetPaldarkAbilitySystemComponent();
	if (ASC == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("APaldarkCharacter::InitAbilitySystem — %s: PlayerState has no Paldark ASC."),
			*GetName());
		return;
	}

	// Standard GAS init: tell the ASC who owns it (PS) and who controls it
	// (this pawn). This MUST happen on both server + autonomous client so
	// prediction works.
	ASC->InitAbilityActorInfo(PS, this);
	CachedAbilitySystemComponent = ASC;

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("APaldarkCharacter::InitAbilitySystem — %s bound ASC %s (owner=%s, auth=%d)."),
		*GetName(),
		*ASC->GetName(),
		*PS->GetName(),
		HasAuthority() ? 1 : 0);

	// W24-25 — Server-only bind: when the AttributeSet broadcasts that
	// Health hit zero, route the kill through the match subsystem so the
	// PlayerState's outcome flips to KIA and the end-condition is
	// re-evaluated. The dummy-target wiring (W9-10) does the same on
	// non-player actors; this is the player path.
	if (HasAuthority() && !MatchDeathHookHandle.IsValid())
	{
		const UPaldarkAttributeSet* AttrConst = PS->GetPaldarkAttributeSet();
		UPaldarkAttributeSet* Attr = const_cast<UPaldarkAttributeSet*>(AttrConst);
		if (Attr != nullptr)
		{
			TWeakObjectPtr<APaldarkPlayerState> WeakPS = PS;
			TWeakObjectPtr<APaldarkCharacter> WeakSelf = this;
			MatchDeathHookHandle = Attr->OnHealthZeroed.AddWeakLambda(
				this,
				[WeakPS, WeakSelf](UAbilitySystemComponent* /*Instigator*/)
				{
					// W37-38 — drop the inventory into a loot bag *before*
					// recording the death so the bag is on the ground by
					// the time the match subsystem may destroy the pawn.
					if (APaldarkCharacter* StrongSelf = WeakSelf.Get())
					{
						StrongSelf->SpawnDeathLootBagFromInventory();
					}
					APaldarkPlayerState* StrongPS = WeakPS.Get();
					if (StrongPS == nullptr)
					{
						return;
					}
					UWorld* PSWorld = StrongPS->GetWorld();
					UPaldarkMatchSubsystem* MatchSub = PSWorld != nullptr
						? PSWorld->GetSubsystem<UPaldarkMatchSubsystem>()
						: nullptr;
					if (MatchSub != nullptr)
					{
						MatchSub->RecordDeath(StrongPS);
					}
				});
			CachedDeathHookAttributeSet = Attr;
		}
	}

	// If SetupPlayerInputComponent already ran (autonomous proxy on a remote
	// client) bind ability inputs now that the ASC is live.
	if (CachedEnhancedInputComponent != nullptr && !bAbilityInputBound)
	{
		BindAbilityInputActions();
	}
}

void APaldarkCharacter::SetPawnData(const UPaldarkPawnData* InPawnData)
{
	CachedPawnData = InPawnData;
	UE_LOG(LogPaldark, Log, TEXT("APaldarkCharacter::SetPawnData — %s cached %s"),
		*GetName(),
		InPawnData != nullptr ? *InPawnData->GetName() : TEXT("<null>"));

	// W27 — Push the AnimInstance class from PawnData onto the inherited mesh.
	// Idempotent: SetAnimInstanceClass with the same class is a no-op inside
	// the engine. Empty soft pointer keeps whatever the BP subclass set —
	// designers can ship a `BP_PaldarkCharacter` with `Mesh.AnimClass` already
	// pointing at `ABP_Paldark_Player` and skip the PawnData field entirely.
	if (InPawnData != nullptr && !InPawnData->DefaultAnimInstanceClass.IsNull())
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			UClass* AnimClass = InPawnData->DefaultAnimInstanceClass.LoadSynchronous();
			if (AnimClass != nullptr && Mesh->GetAnimClass() != AnimClass)
			{
				Mesh->SetAnimInstanceClass(AnimClass);
				UE_LOG(LogPaldark, Log,
					TEXT("APaldarkCharacter::SetPawnData — %s applied AnimInstance class %s from %s."),
					*GetName(),
					*AnimClass->GetName(),
					*InPawnData->GetName());
			}
			else if (AnimClass == nullptr)
			{
				UE_LOG(LogPaldark, Warning,
					TEXT("APaldarkCharacter::SetPawnData — %s failed to load DefaultAnimInstanceClass %s."),
					*GetName(),
					*InPawnData->DefaultAnimInstanceClass.ToString());
			}
		}
	}

	// If the input component was already set up before PawnData arrived (rare
	// race during late-joiners) bind now.
	if (CachedEnhancedInputComponent != nullptr && !bNativeInputBound)
	{
		BindNativeInputActions();
	}
}

void APaldarkCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CachedEnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (CachedEnhancedInputComponent == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("APaldarkCharacter::SetupPlayerInputComponent — %s expects UEnhancedInputComponent (Project Settings → Input → Default Input Component Class)."),
			*GetName());
		return;
	}
	BindNativeInputActions();
	BindAbilityInputActions();
}

void APaldarkCharacter::BindNativeInputActions()
{
	if (CachedEnhancedInputComponent == nullptr || CachedPawnData == nullptr || bNativeInputBound)
	{
		return;
	}
	if (CachedPawnData->InputConfig.IsNull())
	{
		UE_LOG(LogPaldark, Warning, TEXT("APaldarkCharacter::BindNativeInputActions — %s has no InputConfig on PawnData %s."),
			*GetName(),
			*CachedPawnData->GetName());
		return;
	}
	const UPaldarkInputConfig* InputConfig = CachedPawnData->InputConfig.LoadSynchronous();
	if (InputConfig == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("APaldarkCharacter::BindNativeInputActions — failed to load InputConfig %s."),
			*CachedPawnData->InputConfig.ToString());
		return;
	}

	int32 BoundCount = 0;
	if (const UInputAction* MoveIA = InputConfig->FindNativeInputActionForTag(PaldarkGameplayTags::TAG_Paldark_InputTag_Move, /*bLogNotFound*/ false))
	{
		CachedEnhancedInputComponent->BindAction(MoveIA, ETriggerEvent::Triggered, this, &APaldarkCharacter::Input_Move);
		++BoundCount;
	}
	if (const UInputAction* LookIA = InputConfig->FindNativeInputActionForTag(PaldarkGameplayTags::TAG_Paldark_InputTag_Look, /*bLogNotFound*/ false))
	{
		CachedEnhancedInputComponent->BindAction(LookIA, ETriggerEvent::Triggered, this, &APaldarkCharacter::Input_Look);
		++BoundCount;
	}
	if (const UInputAction* JumpIA = InputConfig->FindNativeInputActionForTag(PaldarkGameplayTags::TAG_Paldark_InputTag_Jump, /*bLogNotFound*/ false))
	{
		CachedEnhancedInputComponent->BindAction(JumpIA, ETriggerEvent::Started,   this, &APaldarkCharacter::Input_Jump_Pressed);
		CachedEnhancedInputComponent->BindAction(JumpIA, ETriggerEvent::Completed, this, &APaldarkCharacter::Input_Jump_Released);
		++BoundCount;
	}

	bNativeInputBound = true;
	UE_LOG(LogPaldark, Log, TEXT("APaldarkCharacter::BindNativeInputActions — %s bound %d native action(s) from %s."),
		*GetName(),
		BoundCount,
		*InputConfig->GetName());
}

void APaldarkCharacter::BindAbilityInputActions()
{
	if (CachedEnhancedInputComponent == nullptr || CachedPawnData == nullptr || bAbilityInputBound)
	{
		return;
	}
	if (CachedAbilitySystemComponent == nullptr)
	{
		// ASC not yet resolved — InitAbilitySystem() will retry once PlayerState
		// is available.
		return;
	}
	if (CachedPawnData->InputConfig.IsNull())
	{
		return;
	}
	const UPaldarkInputConfig* InputConfig = CachedPawnData->InputConfig.LoadSynchronous();
	if (InputConfig == nullptr)
	{
		return;
	}

	int32 BoundCount = 0;
	if (const UInputAction* SprintIA = InputConfig->FindAbilityInputActionForTag(PaldarkGameplayTags::TAG_Paldark_InputTag_Sprint, /*bLogNotFound*/ false))
	{
		CachedEnhancedInputComponent->BindAction(SprintIA, ETriggerEvent::Started,   this, &APaldarkCharacter::Input_Sprint_Pressed);
		CachedEnhancedInputComponent->BindAction(SprintIA, ETriggerEvent::Completed, this, &APaldarkCharacter::Input_Sprint_Released);
		CachedEnhancedInputComponent->BindAction(SprintIA, ETriggerEvent::Canceled,  this, &APaldarkCharacter::Input_Sprint_Released);
		++BoundCount;
	}
	if (const UInputAction* FireIA = InputConfig->FindAbilityInputActionForTag(PaldarkGameplayTags::TAG_Paldark_InputTag_Fire, /*bLogNotFound*/ false))
	{
		// W9-10 — Fire is semi-auto: only Started is bound (each click activates
		// the hitscan ability once; full-auto / hold-to-fire is W17-18 polish).
		CachedEnhancedInputComponent->BindAction(FireIA, ETriggerEvent::Started, this, &APaldarkCharacter::Input_Fire_Pressed);
		++BoundCount;
	}

	bAbilityInputBound = true;
	UE_LOG(LogPaldarkGAS, Log,
		TEXT("APaldarkCharacter::BindAbilityInputActions — %s bound %d ability action(s) from %s."),
		*GetName(),
		BoundCount,
		*InputConfig->GetName());
}

void APaldarkCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller == nullptr || Axis.IsNearlyZero())
	{
		return;
	}
	const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right,   Axis.X);
}

void APaldarkCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller == nullptr)
	{
		return;
	}
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void APaldarkCharacter::Input_Jump_Pressed(const FInputActionValue& /*Value*/)
{
	Jump();
}

void APaldarkCharacter::Input_Jump_Released(const FInputActionValue& /*Value*/)
{
	StopJumping();
}

void APaldarkCharacter::Input_Sprint_Pressed(const FInputActionValue& /*Value*/)
{
	if (CachedAbilitySystemComponent == nullptr)
	{
		return;
	}
	CachedAbilitySystemComponent->TryActivateAbilityByActivationTag(
		PaldarkGameplayTags::TAG_Paldark_Ability_Sprint);
}

void APaldarkCharacter::Input_Sprint_Released(const FInputActionValue& /*Value*/)
{
	if (CachedAbilitySystemComponent == nullptr)
	{
		return;
	}
	CachedAbilitySystemComponent->CancelAbilityByActivationTag(
		PaldarkGameplayTags::TAG_Paldark_Ability_Sprint);
}

void APaldarkCharacter::Input_Fire_Pressed(const FInputActionValue& /*Value*/)
{
	if (CachedAbilitySystemComponent == nullptr)
	{
		return;
	}
	// W9-10 — Activate by tag. The HitscanFire ability is ServerOnly, so the
	// ASC RPC's the activation to the server which runs the line trace and
	// applies the damage GE. Clients only see the resulting Health rep.
	CachedAbilitySystemComponent->TryActivateAbilityByActivationTag(
		PaldarkGameplayTags::TAG_Paldark_Ability_Fire);
}

void APaldarkCharacter::SpawnDeathLootBagFromInventory()
{
	if (!HasAuthority())
	{
		// Defensive — the hook is bound server-only, but a forced call from
		// a console command should still respect authority.
		return;
	}
	if (PlayerDeathBagClass == nullptr)
	{
		UE_LOG(LogPaldark, Verbose,
			TEXT("APaldarkCharacter::SpawnDeathLootBagFromInventory — %s has no PlayerDeathBagClass; skipping."),
			*GetName());
		return;
	}

	UPaldarkPlayerInventoryComponent* Inv = InventorySlot;
	if (Inv == nullptr)
	{
		return;
	}

	const TArray<FPaldarkInventoryEntry>& Entries = Inv->GetEntries();
	if (Entries.Num() == 0)
	{
		UE_LOG(LogPaldark, Verbose,
			TEXT("APaldarkCharacter::SpawnDeathLootBagFromInventory — %s inventory empty; no bag spawned."),
			*GetName());
		return;
	}

	// Translate FPaldarkInventoryEntry → FPaldarkLootBagEntry (W33-34 wire
	// format). Nested container `InnerEntries` are spilled flat into the
	// same bag so an unsuspecting picker doesn't have to recurse the
	// container UI; this is intentionally lossy (the W47 save-game
	// reconstructs the original tree).
	TArray<FPaldarkLootBagEntry> Contents;
	Contents.Reserve(Entries.Num());
	for (const FPaldarkInventoryEntry& Row : Entries)
	{
		if (!Row.IsValid()) { continue; }
		FPaldarkLootBagEntry Out;
		Out.ItemDefinition = Row.ItemDef;
		Out.Count          = Row.StackCount;
		Contents.Add(MoveTemp(Out));
		for (const FPaldarkInventoryEntry& Inner : Row.InnerEntries)
		{
			if (!Inner.IsValid()) { continue; }
			FPaldarkLootBagEntry InnerOut;
			InnerOut.ItemDefinition = Inner.ItemDef;
			InnerOut.Count          = Inner.StackCount;
			Contents.Add(MoveTemp(InnerOut));
		}
	}

	if (Contents.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr) { return; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	const FTransform DropXform = GetActorTransform();

	APaldarkLootBag* Bag = World->SpawnActor<APaldarkLootBag>(PlayerDeathBagClass, DropXform, Params);
	if (Bag == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("APaldarkCharacter::SpawnDeathLootBagFromInventory — %s failed to spawn %s."),
			*GetName(),
			*PlayerDeathBagClass->GetName());
		return;
	}
	Bag->InitializeContents(Contents);
	UE_LOG(LogPaldark, Log,
		TEXT("APaldarkCharacter::SpawnDeathLootBagFromInventory — %s dropped %d entries into %s at %s."),
		*GetName(),
		Contents.Num(),
		*Bag->GetName(),
		*DropXform.GetLocation().ToString());

	// Clear the source inventory so the world bag is the canonical copy.
	Inv->DropAllItems();
}
