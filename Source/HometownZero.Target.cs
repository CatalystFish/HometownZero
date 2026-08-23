using UnrealBuildTool;
using System.Collections.Generic;

public class HometownZeroTarget : TargetRules
{
	public HometownZeroTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.Add("HometownZero");
	}
}
