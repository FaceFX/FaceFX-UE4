using UnrealBuildTool;
using System.IO;

public class FaceFXLib : ModuleRules
{
    //used to show warning only once.
    static bool DebugLibsWarningDisplayed = false;
    
    //The folder in the FaceFX runtime is located in. You need to update this whenever you update your FaceFX runtime
    public static string RuntimeFolder { get { return "FaceFx-0.9.10"; } }

    public FaceFXLib(TargetInfo Target)
    {
        Type = ModuleType.External;

        string FaceFXLib;
        string FaceFXDir;
        string FaceFXDirLib;
        if (GetLibs(Target, out FaceFXDir, out FaceFXDirLib, out FaceFXLib))
        {
            PublicIncludePaths.Add(Path.Combine(new[] { FaceFXDir, "common", "src" }));
            PublicLibraryPaths.Add(FaceFXDirLib);
            PublicAdditionalLibraries.Add(FaceFXLib);
        }
    }


    /// <summary>
    /// Gets the libs for FaceFX
    /// </summary>
    /// <param name="Target">The targetinfo to get the libs for</param>
    /// <param name="FaceFXDir">The result facefx directory</param>
    /// <param name="FaceFXDirLib">The result facefx directory for libraries</param>
    /// <param name="FaceFXLib">The actual lib filename</param>
    /// <returns>True if all libs were found, else false</returns>
    private static bool GetLibs(TargetInfo Target, out string FaceFXDir, out string FaceFXDirLib, out string FaceFXLib)
    {
        FaceFXDir = Path.Combine(new []{ "..", "Plugins", "Runtime", "FaceFX", "Source", "FaceFXLib", RuntimeFolder });
        FaceFXDirLib = string.Empty;
        FaceFXLib = string.Empty;

        if (!Directory.Exists(FaceFXDir))
        {
            System.Console.WriteLine(System.String.Format("FaceFX: Cannot find FaceFX Folder '{0}'", FaceFXDir));
            return false;
        }

        //default static lib file
        FaceFXLib = "libfacefx.lib";

        string CompilerFolder = WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013 ? "vs12" : "vs11";

        string PlatformFolder = string.Empty;

        switch (Target.Platform)
        {
            case UnrealTargetPlatform.Win32:
                PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "Win32" });
                break;
            case UnrealTargetPlatform.Win64:
                PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "x64" });
                break;
            case UnrealTargetPlatform.XboxOne:
                PlatformFolder = Path.Combine(new[] { "xboxone", CompilerFolder });
                break;
            case UnrealTargetPlatform.PS4:
                FaceFXLib = "facefx";
                PlatformFolder = Path.Combine(new[] { "ps4", CompilerFolder });
                break;
            default:
                System.Console.WriteLine(System.String.Format("FaceFX disabled. Unsupported target platform: {0}", Target.Platform));
                return false;
        }

        string ConfigFolder = "Release";

        switch (Target.Configuration)
        {
            case UnrealTargetConfiguration.Debug:
                // change bDebugBuildsActuallyUseDebugCRT to true in BuildConfiguration.cs to actually link debug binaries
                if (BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    ConfigFolder = "Debug";
                    if (DebugLibsWarningDisplayed == false)
                    {
                        System.Console.WriteLine("Using debug libs for FaceFX");
                        DebugLibsWarningDisplayed = true;
                    }
                }
                break;
        }

        FaceFXDirLib = System.IO.Path.Combine(new[] { FaceFXDir, "bin", PlatformFolder, ConfigFolder });

        if (!Directory.Exists(FaceFXDirLib))
        {
            System.Console.WriteLine(System.String.Format("FaceFX: Cannot find FaceFX lib Folder '{0}'", FaceFXDirLib));
            return false;
        }

        return true;
    }

    /// <summary>
    /// Gets the indicator if FaceFX can be used on the given target
    /// </summary>
    /// <param name="Target">The target info</param>
    /// <returns>True if can be used, else false</returns>
    public static bool IsSupported(TargetInfo Target)
    {
        string FaceFXDir;
        string FaceFXDirLib;
        string FaceFXLib;
        return GetLibs(Target, out FaceFXDir, out FaceFXDirLib, out FaceFXLib);
    }
}
