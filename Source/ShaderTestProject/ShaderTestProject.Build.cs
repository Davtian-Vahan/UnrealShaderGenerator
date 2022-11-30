// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderTestProject : ModuleRules
{
	public ShaderTestProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Blutility", "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });

        PrivateIncludePaths.AddRange(new string[] { System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private" });

    }
}
