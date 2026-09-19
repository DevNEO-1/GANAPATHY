// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ganapati : ModuleRules
{
	public Ganapati(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Ganapati",
			"Ganapati/Characters",
			"Ganapati/Components",
			"Ganapati/Controllers",
			"Ganapati/GameModes",
			"Ganapati/Environment",
			"Ganapati/NPCs",
			"Ganapati/Enemies",
			"Ganapati/Interaction",
			"Ganapati/UI",
			"Ganapati/Variant_Platforming",
			"Ganapati/Variant_Platforming/Animation",
			"Ganapati/Variant_Combat",
			"Ganapati/Variant_Combat/AI",
			"Ganapati/Variant_Combat/Animation",
			"Ganapati/Variant_Combat/Gameplay",
			"Ganapati/Variant_Combat/Interfaces",
			"Ganapati/Variant_Combat/UI",
			"Ganapati/Variant_SideScrolling",
			"Ganapati/Variant_SideScrolling/AI",
			"Ganapati/Variant_SideScrolling/Gameplay",
			"Ganapati/Variant_SideScrolling/Interfaces",
			"Ganapati/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
