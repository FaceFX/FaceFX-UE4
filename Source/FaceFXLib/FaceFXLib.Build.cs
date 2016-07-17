/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015 OC3 Entertainment, Inc.
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*******************************************************************************/

using UnrealBuildTool;
using System.IO;

public class FaceFXLib : ModuleRules
{
    //used to show warning only once.
    static bool DebugLibsWarningDisplayed = false;
    
    //The folder in the FaceFX runtime is located in. You need to update this whenever you update your FaceFX runtime
    public static string RuntimeFolder { get { return "facefx-runtime-1.1.1/facefx"; } }

    public FaceFXLib(TargetInfo Target)
    {
        Type = ModuleType.External;
        
        string FaceFXLib;
        string FaceFXDir;
        string FaceFXDirLib;
        if (GetLibs(Target, out FaceFXDir, out FaceFXDirLib, out FaceFXLib))
        {
            PublicLibraryPaths.Add(FaceFXDirLib);

            if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                PublicAdditionalLibraries.Add(FaceFXDirLib + "/" + FaceFXLib);
            }
            else
            {
                PublicAdditionalLibraries.Add(FaceFXLib);
            }
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
    private bool GetLibs(TargetInfo Target, out string FaceFXDir, out string FaceFXDirLib, out string FaceFXLib)
    {
        FaceFXDir = Path.Combine(new []{ this.ModuleDirectory, RuntimeFolder });
        FaceFXDirLib = string.Empty;
        FaceFXLib = string.Empty;
        
        if (!Directory.Exists(FaceFXDir))
        {
            System.Console.WriteLine(System.String.Format("FaceFX: Cannot find FaceFX Folder '{0}'", FaceFXDir));
            return false;
        }

        //default static lib file
        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            FaceFXLib = "libfacefx.a";
        }
        else
        {
            FaceFXLib = "libfacefx.lib";
        }

        string CompilerFolder = WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2015 ? "vs14" : "vs12";

        string PlatformFolder = string.Empty;

        switch (Target.Platform)
        {
            case UnrealTargetPlatform.Win32:
                PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "Win32" });
                break;
            case UnrealTargetPlatform.Win64:
                PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "x64" });
                break;
            case UnrealTargetPlatform.Mac:
                PlatformFolder = Path.Combine(new[] { "osx" });
                break;
            //case UnrealTargetPlatform.XboxOne:
            //    PlatformFolder = Path.Combine(new[] { "xboxone", CompilerFolder });
            //    break;
            //case UnrealTargetPlatform.PS4:
            //    FaceFXLib = "facefx";
            //    PlatformFolder = Path.Combine(new[] { "ps4", CompilerFolder });
            //    break;
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
}
