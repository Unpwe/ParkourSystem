// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ParkourSystem : ModuleRules
{
	public ParkourSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"ParkourSystem/Public"
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"ParkourSystem/Private"
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core", 
				"CoreUObject", 
				"Engine", 
				"InputCore",
				"EnhancedInput",
				"GameplayTags",
				"MotionWarping"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
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
