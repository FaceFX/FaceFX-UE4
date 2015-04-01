using UnrealBuildTool;

public class FaceFxEditor : ModuleRules
{
    public FaceFxEditor(TargetInfo Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "EditorStyle",
                "Slate",
                "SlateCore",
                "InputCore",
                "AssetTools",
                "ContentBrowser",
                "MainFrame",
                "DesktopPlatform",
                "AnimGraph",
                "BlueprintGraph",
                "FaceFx"
            }
        );

        DynamicallyLoadedModuleNames.Add("AssetTools");

        Definitions.Add("FACEFX_RUNTIMEFOLDER=\"" + FaceFxLib.RuntimeFolder + "\"");
    }
}
