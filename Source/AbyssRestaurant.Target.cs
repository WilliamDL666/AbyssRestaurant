using UnrealBuildTool;
using System.Collections.Generic;

public class AbyssRestaurantTarget : TargetRules
{
	public AbyssRestaurantTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("AbyssRestaurant");
	}
}
