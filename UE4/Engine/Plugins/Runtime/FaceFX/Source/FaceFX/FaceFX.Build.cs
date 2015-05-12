﻿/*******************************************************************************
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