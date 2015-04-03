using UnrealBuildTool;

public class FaceFX : ModuleRules
{
    public FaceFX(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "TargetPlatform",
                "FaceFXLib"
            }
        );

        PublicIncludePathModuleNames.Add("FaceFXLib");

        //the runtime header define, i.e. "FaceFx-0.9.10/facefx.h"
        Definitions.Add("FACEFX_RUNTIMEHEADER=\"" + FaceFXLib.RuntimeFolder + "/facefx.h\"");
	}

    /// <summary>
    /// Gets the indicator if FaceFX can be used on the given target
    /// </summary>
    /// <param name="Target">The target info</param>
    /// <returns>True if can be used, else false</returns>
    public static bool IsSupported(TargetInfo Target)
    {
        return FaceFXLib.IsSupported(Target);
    }
}