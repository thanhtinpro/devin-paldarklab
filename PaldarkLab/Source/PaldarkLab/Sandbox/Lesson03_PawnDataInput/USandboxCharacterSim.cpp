// USandboxCharacterSim.cpp
#include "USandboxCharacterSim.h"
#include "USandboxPawnData.h"
#include "USandboxInputConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogSandboxInput, Log, All);

void USandboxCharacterSim::SimulateOnPossess(USandboxPawnData* PawnData)
{
    // STEP 1: Cache PawnData — mirrors PC::OnPossess line 138
    CachedPawnData = PawnData;
    if (PawnData == nullptr)
    {
        UE_LOG(LogSandboxInput, Warning, TEXT("[OnPossess] No PawnData!"));
        return;
    }
    UE_LOG(LogSandboxInput, Log, TEXT("[OnPossess] PawnData=%s PawnClass=%s"),
        *PawnData->GetName(), *PawnData->PawnClassName);

    // STEP 2: SetPawnData + Bind input — mirrors Character::SetupPlayerInputComponent
    if (PawnData->InputConfig != nullptr)
    {
        BindNativeInputActions(PawnData->InputConfig);
        BindAbilityInputActions(PawnData->InputConfig);
    }
    else
    {
        UE_LOG(LogSandboxInput, Warning, TEXT("[OnPossess] PawnData has no InputConfig!"));
    }

    // STEP 3: Apply mapping contexts — mirrors PC::ApplyMappingContexts
    ApplyMappingContexts(PawnData);
}

void USandboxCharacterSim::BindNativeInputActions(const USandboxInputConfig* Config)
{
    if (Config == nullptr || bNativeInputBound) return;

    // SANDBOX: Trong thực tế, iterate known tags and FindNativeInputActionForTag
    // Mirrors PaldarkCharacter.cpp line 381-396
    struct FTagHandler { FGameplayTag Tag; FString HandlerName; };
    // Register sandbox tags for lookup
    // NOTE: Tags must be registered before use. We use RequestGameplayTag here.
    TArray<FTagHandler> KnownNative = {
        { FGameplayTag::RequestGameplayTag(FName("Sandbox.InputTag.Move")), TEXT("Input_Move") },
        { FGameplayTag::RequestGameplayTag(FName("Sandbox.InputTag.Look")), TEXT("Input_Look") },
        { FGameplayTag::RequestGameplayTag(FName("Sandbox.InputTag.Jump")), TEXT("Input_Jump") },
    };

    int32 BoundCount = 0;
    for (const FTagHandler& TH : KnownNative)
    {
        FString ActionName = Config->FindNativeActionForTag(TH.Tag, false);
        if (!ActionName.IsEmpty())
        {
            NativeBindings.Add(TH.Tag.ToString(), TH.HandlerName);
            ++BoundCount;
            UE_LOG(LogSandboxInput, Log, TEXT("[BindNative] %s → %s (action=%s)"),
                *TH.Tag.ToString(), *TH.HandlerName, *ActionName);
        }
    }

    bNativeInputBound = true;
    UE_LOG(LogSandboxInput, Log, TEXT("[BindNative] Bound %d native action(s)"), BoundCount);
}

void USandboxCharacterSim::BindAbilityInputActions(const USandboxInputConfig* Config)
{
    if (Config == nullptr || bAbilityInputBound) return;

    // Mirrors PaldarkCharacter.cpp line 428-441
    struct FTagHandler { FGameplayTag Tag; FString HandlerName; };
    TArray<FTagHandler> KnownAbility = {
        { FGameplayTag::RequestGameplayTag(FName("Sandbox.InputTag.Sprint")), TEXT("Input_Sprint") },
        { FGameplayTag::RequestGameplayTag(FName("Sandbox.InputTag.Fire")), TEXT("Input_Fire") },
    };

    int32 BoundCount = 0;
    for (const FTagHandler& TH : KnownAbility)
    {
        FString ActionName = Config->FindAbilityActionForTag(TH.Tag, false);
        if (!ActionName.IsEmpty())
        {
            AbilityBindings.Add(TH.Tag.ToString(), TH.HandlerName);
            ++BoundCount;
            UE_LOG(LogSandboxInput, Log, TEXT("[BindAbility] %s → %s (action=%s)"),
                *TH.Tag.ToString(), *TH.HandlerName, *ActionName);
        }
    }

    bAbilityInputBound = true;
    UE_LOG(LogSandboxInput, Log, TEXT("[BindAbility] Bound %d ability action(s)"), BoundCount);
}

void USandboxCharacterSim::ApplyMappingContexts(const USandboxPawnData* PawnData)
{
    // Mirrors PC::ApplyMappingContexts — sort by priority, add to subsystem
    // SANDBOX: just log them
    ActiveMappingContexts.Reset();
    for (const FSandboxMappingContext& MC : PawnData->MappingContexts)
    {
        ActiveMappingContexts.Add(MC.MappingContextName);
        UE_LOG(LogSandboxInput, Log, TEXT("[MappingContext] Added '%s' at priority %d"),
            *MC.MappingContextName, MC.Priority);
    }
    UE_LOG(LogSandboxInput, Log, TEXT("[MappingContext] Applied %d IMC(s)"), ActiveMappingContexts.Num());
}

FString USandboxCharacterSim::GetBoundHandlerForTag(const FGameplayTag& Tag) const
{
    FString TagStr = Tag.ToString();
    if (const FString* Handler = NativeBindings.Find(TagStr)) return *Handler;
    if (const FString* Handler = AbilityBindings.Find(TagStr)) return *Handler;
    return TEXT("");
}

TArray<FString> USandboxCharacterSim::GetActiveMappingContexts() const { return ActiveMappingContexts; }
int32 USandboxCharacterSim::GetNativeBindCount() const { return NativeBindings.Num(); }
int32 USandboxCharacterSim::GetAbilityBindCount() const { return AbilityBindings.Num(); }
const USandboxPawnData* USandboxCharacterSim::GetPawnData() const { return CachedPawnData; }

FString USandboxCharacterSim::SimulateInputPress(const FGameplayTag& InputTag)
{
    FString TagStr = InputTag.ToString();
    if (const FString* Handler = NativeBindings.Find(TagStr))
    {
        return FString::Printf(TEXT("NATIVE: %s → %s(Pressed)"), *TagStr, **Handler);
    }
    if (const FString* Handler = AbilityBindings.Find(TagStr))
    {
        return FString::Printf(TEXT("ABILITY: %s → ASC->TryActivateAbilityByTag(%s)"), *TagStr, **Handler);
    }
    return FString::Printf(TEXT("UNBOUND: %s has no handler"), *TagStr);
}

FString USandboxCharacterSim::SimulateInputRelease(const FGameplayTag& InputTag)
{
    FString TagStr = InputTag.ToString();
    if (const FString* Handler = AbilityBindings.Find(TagStr))
    {
        return FString::Printf(TEXT("ABILITY: %s → ASC->CancelAbilityByTag(%s)"), *TagStr, **Handler);
    }
    return FString::Printf(TEXT("RELEASE: %s (no cancel needed)"), *TagStr);
}
