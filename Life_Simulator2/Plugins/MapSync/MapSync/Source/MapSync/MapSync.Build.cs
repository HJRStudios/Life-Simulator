// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MapSync : ModuleRules
{
	public MapSync(TargetInfo Target)
	{
		// PublicIncludePaths.AddRange(new string[] {"MapSync/Public"});
        PrivateIncludePaths.AddRange(new string[] {"MapSync/Private"});

		PublicDependencyModuleNames.AddRange(new string[] 
        {
            "Core",
            "Sockets",
            "Networking"
        });
        PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
            "EditorStyle",
			"LevelEditor"
		});
	}
}
