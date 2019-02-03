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

#include "FaceFX.h"
#include "FaceFXContext.h"
#include "FaceFXConfig.h"
#include "FaceFXAnim.h"
#include "Modules/ModuleManager.h"
#include "Engine/StreamableManager.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#endif //WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaceFX"

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

bool FaceFX::Check(int32 Value)
{
	return Value != 0 && (ffx_errno() == EOK);
}

FString FaceFX::GetFaceFXError()
{
	char Msg[128] = { 0 };
	if (ffx_strerror(ffx_errno(), Msg, 128))
	{
		return FString(Msg);
	}
	else
	{
		return TEXT("Unable to retrieve additional FaceFX error message.");
	}
}

ffx_anim_handle_t* FaceFX::LoadAnimation(const FFaceFXAnimData& AnimData)
{
	//load animations
	if (AnimData.RawData.Num() == 0)
	{
		UE_LOG(LogFaceFX, Warning, TEXT("FaceFX::LoadAnimation. Missing facefx animation data."));
		return nullptr;
	}

	ffx_context_t Context = FFaceFXContext::CreateContext();

	ffx_anim_handle_t* NewHandle = nullptr;
	if (!FaceFX::Check(ffx_create_anim_handle((char*)(&AnimData.RawData[0]), AnimData.RawData.Num(), FFX_RUN_INTEGRITY_CHECK, &NewHandle, &Context)) || !NewHandle)
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::LoadAnimation. Unable to create animation FaceFX handle. %s"), *FaceFX::GetFaceFXError());
	}

	return NewHandle;
}

bool FaceFX::GetAnimationBounds(const UFaceFXAnim* Animation, float& OutStart, float& OutEnd)
{
	ffx_anim_handle_t* AnimHandle = FaceFX::LoadAnimation(Animation->GetData());
	if (!AnimHandle)
	{
		return false;
	}

	bool Result = true;
	if (!FaceFX::Check(ffx_get_anim_bounds(AnimHandle, &OutStart, &OutEnd)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::GetAnimationBounds. FaceFX call <ffx_get_anim_bounds> failed. %s. Asset: %s"), *FaceFX::GetFaceFXError(), *GetNameSafe(Animation));
		Result = false;
	}

	ffx_destroy_anim_handle(&AnimHandle, nullptr, nullptr);
	AnimHandle = nullptr;

	return Result;
}

#if WITH_EDITOR
void RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "FaceFX - Game",
			LOCTEXT("FaceFXSettingsNameGame", "FaceFX - Game"),
			LOCTEXT("FaceFXSettingsDescriptionGame", "Configure FaceFX game settings"),
			GetMutableDefault<UFaceFXConfig>());
	}
}

void UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "FaceFX - Game");
	}
}

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

		RegisterSettings();
	}

	virtual void ShutdownModule() override
	{
		UnregisterSettings();
	}
};
IMPLEMENT_MODULE(FFaceFXModule, FaceFX);
#else
IMPLEMENT_MODULE(FDefaultModuleImpl, FaceFX);
#endif

#undef LOCTEXT_NAMESPACE