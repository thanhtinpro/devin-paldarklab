// PALDARK W1 — Core module: log categories, shared types. Loads at PreDefault phase
// so every other module (Game, Editor, plugins) can use LogPaldark* immediately.

using UnrealBuildTool;

public class PaldarkLabCore : ModuleRules
{
	public PaldarkLabCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			// W1 day 8-10: PaldarkGameplayTags.h uses NativeGameplayTags.h and exports
			// FNativeGameplayTag handles to downstream modules.
			"GameplayTags"
		});
	}
}
