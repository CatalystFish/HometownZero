using UnrealBuildTool;

public class HometownEditor : ModuleRules
{
	public HometownEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Projects"
		});

		// OSM ingestion work (Phase 0 Path B) will enable exceptions here:
		// bEnableExceptions = true;  // required by protozero/FastXml error paths
	}
}
