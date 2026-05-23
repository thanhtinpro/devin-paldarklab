// PALDARK W1 — Editor target. See README.md and Documents/UE5_Course/P01_CPP_Build.md.

using UnrealBuildTool;
using System.Collections.Generic;

public class PaldarkLabEditorTarget : TargetRules
{
	public PaldarkLabEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;

		ExtraModuleNames.AddRange(new string[]
		{
			"PaldarkLabCore",
			"PaldarkLab",
			"PaldarkLabEditor"
		});
	}
}
