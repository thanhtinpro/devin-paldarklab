// PALDARK W1 — Game target. See README.md and Documents/UE5_Course/P01_CPP_Build.md.

using UnrealBuildTool;
using System.Collections.Generic;

public class PaldarkLabTarget : TargetRules
{
	public PaldarkLabTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;

		ExtraModuleNames.AddRange(new string[]
		{
			"PaldarkLabCore",
			"PaldarkLab"
		});
	}
}
