using UnrealBuildTool;

public class CustomGit : ModuleRules
{
	public CustomGit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"SourceControl",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"ToolMenus",
				"InputCore",
				"EditorStyle",
				"WorkspaceMenuStructure",
				"EditorWidgets",
				"UnrealEd",
			}
		);
	}
}
