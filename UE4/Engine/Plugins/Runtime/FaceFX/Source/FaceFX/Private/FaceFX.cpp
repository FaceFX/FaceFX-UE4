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

#include FACEFX_RUNTIMEHEADER

#include "FaceFX.h"
#include "ModuleManager.h"
#include "Engine/StreamableManager.h"

DEFINE_LOG_CATEGORY(LogFaceFX);

/**
* Gets the FaceFX version
* @returns The FaceFX version string
*/
FString FaceFX::GetVersion()
{
	char ffx_version_string[32];
	ffx_strversion(ffx_version_string, 32);
	
	return FString(ANSI_TO_TCHAR(ffx_version_string));
}

/**
* Gets the FaceFX platform
* @returns The FaceFX platform string
*/
FString FaceFX::GetPlatform()
{
	ffx_platform_info_t ffx_platform;
	ffx_platform_info(&ffx_platform);

	char ffx_platform_string[32];
	ffx_strplatform(&ffx_platform, ffx_platform_string, 32);

	return FString(ANSI_TO_TCHAR(ffx_platform_string));
}

/**
* Gets the global streamer for FaceFX assets
* @returns The streamer
*/
FStreamableManager& FaceFX::GetStreamer()
{
	/** the global streamer for FaceFX assets */
	static FStreamableManager s_streamer;
	return s_streamer;
}

IMPLEMENT_MODULE(FDefaultModuleImpl, FaceFX);