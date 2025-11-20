// Copyright 2025, Aquanox.

using UnrealBuildTool;

public class EnhancedAsyncActionEditor : ModuleRules
{
	// This is to emulate engine installation and verify includes during development
	// Gives effect similar to BuildPlugin with -StrictIncludes
	public bool bStrictIncludesCheck = false;

	public EnhancedAsyncActionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		if (bStrictIncludesCheck)
		{
			bUseUnity = false;
			PCHUsage = PCHUsageMode.NoPCHs;
			// Enable additional checks used for Engine modules
			bTreatAsEngineModule = true;
		}

		// Disable Private/Public structure.
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
				"PropertyEditor",
				"ToolMenus",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);
	}
}
