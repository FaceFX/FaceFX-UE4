// Copyright 2014 Yager GmbH All Rights Reserved.

using UnrealBuildTool;

public class FaceFx : ModuleRules
{
    public FaceFx(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "FaceFxLib"
            }
        );

        if (UEBuildConfiguration.bBuildEditor)
        {
            //Required for the Animation Group Selection dialogue when loading animation sets
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Slate",
                    "SlateCore",
                    "InputCore", 
                    "EditorStyle",
                    "UnrealEd",
                    "AssetTools"
                });
        }

        PrivateIncludePathModuleNames.AddRange(new string[] { "TargetPlatform", "FaceFxLib" });
	}

    /// <summary>
    /// Gets the indicator if FaceFX can be used on the given target
    /// </summary>
    /// <param name="Target">The target info</param>
    /// <returns>True if can be used, else false</returns>
    public static bool IsSupported(TargetInfo Target)
    {
        return FaceFxLib.IsSupported(Target);
    }
}