// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
using UnrealBuildTool;

public class MCP : ModuleRules
{
	public MCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {

			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {

			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities",
				"HTTP",
				"Sockets",
				"Networking"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"ContentBrowser",
				"EditorStyle",
				"Slate",
				"SlateCore",
				"HTTPServer",
				"HTTP",
				"Json",
				"JsonUtilities"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
// From Penguin Assistant End
