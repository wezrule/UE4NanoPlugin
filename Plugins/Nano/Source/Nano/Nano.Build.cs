// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Nano : ModuleRules
{
	public Nano(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDefinitions.Add("ED25519_CUSTOMRNG");
		PublicDefinitions.Add("ED25519_CUSTOMHASH");

		bEnableExceptions = true;

		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
			PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
		}

		// To remove warnings like: error C4668: '__clang_major___WORKAROUND_GUARD' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		bEnableUndefinedIdentifierWarnings = false;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Http",
                "Json",
                "JsonUtilities",
                "WebSockets"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
