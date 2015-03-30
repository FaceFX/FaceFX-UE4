// Copyright 2013 Yager GmbH All Rights Reserved.

using UnrealBuildTool;

public class FaceFxUI : ModuleRules
{
    public FaceFxUI(TargetInfo Target)
    {
        Definitions.Add("WITH_EDITOR=1");
        Definitions.Add("WITH_EDITORONLY_DATA=1");

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Slate",
                "SlateCore",
                "AssetTools",
                "ContentBrowser",
                "UnrealEd",
                "FaceFx",
                "EditorStyle",
                "MainFrame",
                "DesktopPlatform",
            }
        );

        PrivateIncludePaths.Add("Runtime/FaceFxUI/Classes");
    }
}
