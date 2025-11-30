// Copyright CattleGame. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintSerializerEditor : ModuleRules
{
	public BlueprintSerializerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"BlueprintGraph",
			"Kismet",
			"Json",
			"JsonUtilities",
			"EditorSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"BlueprintSerializer",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"InputCore",
			"PropertyEditor",
			"AssetTools",
			"ContentBrowser",
			"ToolMenus",
			"Merge",
			"JsonObjectGraph"
		});
	}
}
