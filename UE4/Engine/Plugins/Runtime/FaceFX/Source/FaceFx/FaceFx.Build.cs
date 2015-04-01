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
                "TargetPlatform",
                "FaceFxLib"
            }
        );

        PublicIncludePathModuleNames.Add("FaceFxLib");

        //the runtime header define, i.e. "FaceFx-0.9.10/facefx.h"
        Definitions.Add("FACEFX_RUNTIMEHEADER=\"" + FaceFxLib.RuntimeFolder + "/facefx.h\"");
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