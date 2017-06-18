/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2017 OC3 Entertainment, Inc.
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

public class FaceFX : ModuleRules
{
    /// <summary>
    /// Indicator if the FaceFX plugin shall be compiled with Wwise being linked in. Default value: false
    /// Target audio system can then be selected at runtime via CVar: FaceFX.PreferredAudioSystem (within FaceFXAudio.cpp)
    /// Default target audio system will be set to Wwise if bCompileWithWwise is set to true
    /// </summary>
    private static bool bCompileWithWwise = true;

    public FaceFX(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = false;

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "AnimGraphRuntime",
                "MovieScene",
                "FaceFXLib",
            }
        );

        if (UEBuildConfiguration.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("TargetPlatform");
        }

        PublicIncludePathModuleNames.Add("FaceFXLib");

        if(bCompileWithWwise)
        {
            PrivateDependencyModuleNames.Add("AkAudio");
        }
        Definitions.Add(string.Format("WITH_WWISE={0}", bCompileWithWwise ? "1" : "0"));
    }
}
