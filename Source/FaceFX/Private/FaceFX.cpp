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

#include "FaceFX.h"
#include "ModuleManager.h"
#include "Engine/StreamableManager.h"

DEFINE_LOG_CATEGORY(LogFaceFX);

FString FaceFX::GetVersion()
{
	char ffx_version_string[32];
	ffx_strversion(ffx_version_string, 32);
	
	return FString(ANSI_TO_TCHAR(ffx_version_string));
}

FString FaceFX::GetPlatform()
{
	ffx_platform_info_t ffx_platform;
	ffx_platform_info(&ffx_platform);

	char ffx_platform_string[32];
	ffx_strplatform(&ffx_platform, ffx_platform_string, 32);

	return FString(ANSI_TO_TCHAR(ffx_platform_string));
}

FStreamableManager& FaceFX::GetStreamer()
{
	/** the global streamer for FaceFX assets */
	static FStreamableManager s_streamer;
	return s_streamer;
}

#if WITH_EDITOR
class FFaceFXModule : public FDefaultModuleImpl
{
	virtual void StartupModule() override
	{
		if(!GIsEditor)
		{
			//Workaround for the circumstance that we have the anim graph node inside an editor only plugin and we can't load the plugin for editor AND uncooked but not during cooked
			//Instead we explicitly load the plugin for this scenario when we play with uncooked standalone
			//That looks like an engine bug. See: https://udn.unrealengine.com/questions/247518/editor-only-plugin-modules.html
			FModuleManager::LoadModuleChecked<FDefaultModuleImpl>("FaceFXEditor");
		}
	}
};
IMPLEMENT_MODULE(FFaceFXModule, FaceFX);
#else
IMPLEMENT_MODULE(FDefaultModuleImpl, FaceFX);
#endif
