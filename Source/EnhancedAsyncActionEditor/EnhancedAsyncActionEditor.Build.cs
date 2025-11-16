// Copyright 2025, Aquanox.

using UnrealBuildTool;

public class EnhancedAsyncActionEditor : ModuleRules
{
	public EnhancedAsyncActionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"EnhancedAsyncAction",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"EditorFramework",
				"ToolMenus",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);
	}
}
