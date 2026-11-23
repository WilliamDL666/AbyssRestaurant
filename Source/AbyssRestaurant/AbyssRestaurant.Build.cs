using UnrealBuildTool;

public class AbyssRestaurant : ModuleRules
{
	public AbyssRestaurant(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false; // several .cpp files share anonymous-namespace helper names

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
