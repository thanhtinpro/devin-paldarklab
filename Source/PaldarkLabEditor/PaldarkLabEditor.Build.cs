// PALDARK W1 — Editor-only module placeholder. Built only by the Editor target. The
// runtime Game and Server targets never see this — keeps editor utilities (asset
// validators, custom asset actions) out of cooked builds.

using UnrealBuildTool;

public class PaldarkLabEditor : ModuleRules
{
	public PaldarkLabEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetTools",
			"PaldarkLab",
			"PaldarkLabCore"
		});
	}
}
