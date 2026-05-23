// PALDARK W1 — Dedicated Server target. Used from W11 onward (see P07_Dedicated_Server.md).
// Game module list is identical to Game target; UnrealBuildTool strips client-only modules
// via TargetType.Server.

using UnrealBuildTool;
using System.Collections.Generic;

public class PaldarkLabServerTarget : TargetRules
{
	public PaldarkLabServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;

		// Dedicated server has no Slate/UI — saves cook time and binary size.
		bUseLoggingInShipping = true;

		ExtraModuleNames.AddRange(new string[]
		{
			"PaldarkLabCore",
			"PaldarkLab"
		});
	}
}
