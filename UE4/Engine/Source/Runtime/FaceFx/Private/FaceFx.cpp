#include "FaceFx-0.9.10/facefx.h"

#include "FaceFx.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY(LogFaceFx);

/** the global streamer for FaceFX assets */
FStreamableManager FaceFx::s_streamer;

/**
* Gets the FaceFX version
* @returns The FaceFX version string
*/
FString FaceFx::GetVersion()
{
	char ffx_version_string[32];
	ffx_strversion(ffx_version_string, 32);
	
	return FString(ANSI_TO_TCHAR(ffx_version_string));
}

/**
* Gets the FaceFX platform
* @returns The FaceFX platform string
*/
FString FaceFx::GetPlatform()
{
	ffx_platform_info_t ffx_platform;
	ffx_platform_info(&ffx_platform);

	char ffx_platform_string[32];
	ffx_strplatform(&ffx_platform, ffx_platform_string, 32);

	return FString(ANSI_TO_TCHAR(ffx_platform_string));
}

IMPLEMENT_MODULE(FDefaultModuleImpl, FaceFx);