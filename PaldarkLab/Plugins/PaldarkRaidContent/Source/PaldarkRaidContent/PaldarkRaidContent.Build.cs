// PALDARK W27-28 — Build script for the PaldarkRaidContent game-feature plugin.
//
// Game-feature plugins ship as standalone runtime modules; this file declares
// the dependencies needed for the plugin's module to compile + the UE
// `ModularGameplay` / `GameFeatures` runtime services it sits on top of.
//
// Keep the dependency list tight — game-feature plugins are loaded later than
// the main project module, so adding heavy dependencies here delays plugin
// activation time. If a new dependency is required by a feature action, add
// it to the matching list (`PublicDependencyModuleNames` for headers shared
// with other features; `PrivateDependencyModuleNames` for implementation-only).

using UnrealBuildTool;

public class PaldarkRaidContent : ModuleRules
{
	public PaldarkRaidContent(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			"PaldarkRaidContent/Public"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameFeatures",
			"ModularGameplay",
			"GameplayAbilities",
			"GameplayTags",
			"PaldarkLab"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
