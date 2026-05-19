// PALDARK W1 — Primary game module. Depends on PaldarkLabCore for log categories.
// Plugin dependencies (GAS, EnhancedInput) declared here so they cook into Game/Server
// builds. Add new plugins to PaldarkLab.uproject Plugins[] first, then list here.

using UnrealBuildTool;

public class PaldarkLab : ModuleRules
{
	public PaldarkLab(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			"PaldarkLab/Public"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameFeatures",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NetCore",
			"PaldarkLabCore",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",

			// W42-43 — AWS backend integration.
			//   HTTP             — IHttpRequest / FHttpModule (used by UPaldarkBackendSubsystem).
			//   Json             — TJsonReader / TJsonWriter (raw JSON parsing if needed).
			//   JsonUtilities    — FJsonObjectConverter::JsonObjectStringToUStruct round-trip.
			//   DeveloperSettings — UPaldarkBackendSettings base class (Project Settings UI).
			"HTTP",
			"Json",
			"JsonUtilities",
			"DeveloperSettings",

			// W44-45 — Hub UI (Pal Stable + Marketplace).
			//   UMG              — UUserWidget base class for the stable + marketplace
			//                     widget C++ subclasses (designer fills in layout).
			"UMG"
		});
	}
}
