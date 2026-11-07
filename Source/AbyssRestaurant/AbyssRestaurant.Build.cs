using UnrealBuildTool;

public class AbyssRestaurant : ModuleRules
{
	public AbyssRestaurant(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ProceduralMeshComponent",
			"Slate",
			"SlateCore",
			"RenderCore"
		});
	}
}
