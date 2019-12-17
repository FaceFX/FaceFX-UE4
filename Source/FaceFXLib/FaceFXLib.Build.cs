/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2019 OC3 Entertainment, Inc. All rights reserved.
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
    public static string RuntimeFolder { get { return "facefx-runtime-1.5.1/facefx"; } }

    public FaceFXLib(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = false;

        Type = ModuleType.External;

        string FaceFXLib;
        string FaceFXDir;
        string FaceFXDirLib;

        GetLibs(Target, out FaceFXDir, out FaceFXDirLib, out FaceFXLib);

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // Arch being empty means use the default plugin Android architectures (armv7+arm64).
            string Arch = Target.Architecture.ToLower();

            if (Arch == "") {
                PublicAdditionalLibraries.Add(FaceFXDirLib + "/armeabi-v7a/" + FaceFXLib);
                PublicAdditionalLibraries.Add(FaceFXDirLib + "/arm64-v8a/" + FaceFXLib);
            } else {
                throw new BuildException(System.String.Format("FaceFX: unexpected non-default Android plugin architecture '{0}'", Arch));
            }
        }
        else
        {
            PublicAdditionalLibraries.Add(FaceFXDirLib + "/" + FaceFXLib);
        }
    }

    /// <summary>
    /// Gets the libs for FaceFX
    /// </summary>
    /// <param name="Target">The targetinfo to get the libs for</param>
    /// <param name="FaceFXDir">The result facefx directory</param>
    /// <param name="FaceFXDirLib">The result facefx directory for libraries</param>
    /// <param name="FaceFXLib">The actual lib filename</param>
    private void GetLibs(ReadOnlyTargetRules Target, out string FaceFXDir, out string FaceFXDirLib, out string FaceFXLib)
    {
        FaceFXDir = Path.Combine(new []{ this.ModuleDirectory, RuntimeFolder });
        FaceFXDirLib = string.Empty;
        FaceFXLib = string.Empty;

        if (!Directory.Exists(FaceFXDir))
        {
            throw new BuildException(System.String.Format("FaceFX: cannot find the FaceFX Runtime directory '{0}'", FaceFXDir));
        }

        string CompilerFolder = "vs14";

        if (Target.WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2017
         || Target.WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2019)
        {
            CompilerFolder = "vs15";
        }

        string PlatformFolder = string.Empty;

        if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            FaceFXLib = "libfacefx.lib";
            PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "Win32" });
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            FaceFXLib = "libfacefx.lib";
            PlatformFolder = Path.Combine(new[] { "windows", CompilerFolder, "x64" });
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            FaceFXLib = "libfacefx.a";
            PlatformFolder = Path.Combine(new[] { "osx" });
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            FaceFXLib = "libfacefx.a";
            PlatformFolder = Path.Combine(new[] { "ios" });
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            FaceFXLib = "libfacefx.a";
            PlatformFolder = Path.Combine(new[] { "android/gnustl_shared" });
        }
        else if (Target.Platform == UnrealTargetPlatform.XboxOne)
        {
            FaceFXLib = "libfacefx.lib";
            PlatformFolder = Path.Combine(new[] { "xboxone", CompilerFolder });
        }
        else if (Target.Platform == UnrealTargetPlatform.PS4)
        {
            FaceFXLib = "libfacefx.a";
            PlatformFolder = Path.Combine(new[] { "ps4", CompilerFolder });
        }
        else if (Target.Platform == UnrealTargetPlatform.Switch)
        {
            FaceFXLib = "libfacefx.a";
            PlatformFolder = Path.Combine(new[] { "nx", CompilerFolder, "NX64" });
        }
        else
        {
            throw new BuildException(System.String.Format("FaceFX: unsupported target platform '{0}'", Target.Platform));
        }

        string ConfigFolder = "Release";

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            // Simulator not supported.
            ConfigFolder = "Release-iphoneos";
        }

        switch (Target.Configuration)
        {
            case UnrealTargetConfiguration.Debug:
                // change bDebugBuildsActuallyUseDebugCRT to true in BuildConfiguration.cs to actually link debug binaries
                if (Target.bDebugBuildsActuallyUseDebugCRT)
                {
                    ConfigFolder = "Debug";

                    if (Target.Platform == UnrealTargetPlatform.IOS)
                    {
                        // Simulator not supported.
                        ConfigFolder = "Debug-iphoneos";
                    }

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
            throw new BuildException(System.String.Format("FaceFX: cannot find the FaceFX Runtime library folder '{0}'", FaceFXDirLib));
        }
    }
}
