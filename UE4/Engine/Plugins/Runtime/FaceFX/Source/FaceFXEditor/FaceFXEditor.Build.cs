using UnrealBuildTool;

public class FaceFXEditor : ModuleRules
{
    public FaceFXEditor(TargetInfo Target)
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
                "FaceFX"
            }
        );

        DynamicallyLoadedModuleNames.Add("AssetTools");

        Definitions.Add("FACEFX_RUNTIMEFOLDER=\"" + FaceFXLib.RuntimeFolder + "\"");
    }
}
