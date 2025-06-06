// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class InfernaleTesting : ModuleRules
{
	public InfernaleTesting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject", "Engine", "RHI", "RenderCore", "InputCore", "EnhancedInput",
			"AdvancedSessions", "AdvancedSessions",
			"UMG", "Slate", "SlateCore", "Niagara", "MassEntity", "MoviePlayer", 
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			// AI/MassAI Plugin Modules
			"MassAIBehavior",
			"MassAIDebug",

			// Runtime/MassEntity Plugin Modules
			"MassEntity",

			// Runtime/MassGameplay Plugin Modules
			"MassActors",
			"MassCommon",
			"MassGameplayDebug",
			"MassLOD",
			"MassMovement",
			"MassNavigation",
			"MassRepresentation",
			"MassReplication",
			"MassSpawner",
			"MassSimulation",
			"MassSignals",
			
			"NetCore",

			"StateTreeModule",
	        
			"StructUtils",

			"VaRest",
            
			"Niagara",
            
			"NavigationSystem",
			"Json",
			"JsonUtilities"
		});
		
		DynamicallyLoadedModuleNames.AddRange(new string[] {
			
			// SteamSession Plugin Modules
			"NetworkReplayStreaming", "InMemoryNetworkReplayStreaming", "OnlineSubsystemSteam",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}