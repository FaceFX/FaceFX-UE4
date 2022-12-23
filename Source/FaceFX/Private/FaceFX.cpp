/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2023 OC3 Entertainment, Inc. All rights reserved.
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
#include "FaceFXAllocator.h"
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
	char VersionString[32];

	FxResult Result = fxGetVersionString(VersionString, 32);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::GetVersion. Unable to get version string. %s"), *FaceFX::GetFaceFXResultString(Result));
	}

	return FString(ANSI_TO_TCHAR(VersionString));
}

FStreamableManager& FaceFX::GetStreamer()
{
	/** the global streamer for FaceFX assets */
	static FStreamableManager s_streamer;
	return s_streamer;
}

FString FaceFX::GetFaceFXResultString(FxResult Result)
{
	switch (Result)
	{
		case FX_SUCCESS:                    return TEXT("Success"); break;
		case FX_WARNING_LEGACY_DATA_FORMAT: return TEXT("Legacy data format loaded. Please recompile the content."); break;
		case FX_ERROR_INVALID_ARGUMENT:     return TEXT("Invalid Argument"); break;
		case FX_ERROR_DATA:                 return TEXT("Invalid Data"); break;
		case FX_ERROR_INCOMPATIBLE_VERSION: return TEXT("Incompatible Data Version"); break;
		case FX_ERROR_INCOMPATIBLE_TYPE:    return TEXT("Incompatible Type"); break;
		case FX_ERROR_SIZE:                 return TEXT("Invalid Data Size"); break;
		case FX_ERROR_RANGE:                return TEXT("Range Error"); break;
		case FX_ERROR_VALIDATION_FAILED:    return TEXT("Data Validation Failed (Corrupt Data)"); break;
		case FX_ERROR_INCOMPATIBLE_HANDLE:  return TEXT("Incompatible Handles"); break;
		case FX_ERROR_ZOMBIE_HANDLE:        return TEXT("Zombie Handle"); break;
		case FX_ERROR_NOT_PERMITTED:        return TEXT("Operation Not Permitted"); break;
		case FX_ERROR_UNKNOWN:              return TEXT("Unknown Error"); break;
		default: 							return TEXT("Unexpected FxResult Value"); break;
	}
}

FxAnimation FaceFX::LoadAnimation(const FFaceFXAnimData& AnimData)
{
	if (AnimData.RawData.Num() == 0)
	{
		UE_LOG(LogFaceFX, Warning, TEXT("FaceFX::LoadAnimation. Missing FaceFX animation data."));
		return FX_INVALID_ANIMATION;
	}

	FxAllocationCallbacks Allocator = FFaceFXAllocator::CreateAllocator();

	FxAnimation Animation = FX_INVALID_ANIMATION;

	FxResult Result = fxAnimationCreate(&AnimData.RawData[0], AnimData.RawData.Num(), FX_DATA_VALIDATION_ON, &Animation, &Allocator);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::LoadAnimation. Unable to create FaceFX animation. %s"), *FaceFX::GetFaceFXResultString(Result));
	}
	else if (Result == FX_WARNING_LEGACY_DATA_FORMAT)
	{
		UE_LOG(LogFaceFX, Verbose, TEXT("FaceFX::LoadAnimation. Loaded a legacy data format. Please recompile the content with the latest FaceFX Runtime compiler."));
	}

	return Animation;
}

bool FaceFX::GetAnimationBounds(const UFaceFXAnim* pAnimation, float& Start, float& End)
{
	FxAnimation Animation = FaceFX::LoadAnimation(pAnimation->GetData());

	if (Animation == FX_INVALID_ANIMATION)
	{
		return false;
	}

	FxResult BoundsResult = fxAnimationGetBounds(Animation, &Start, &End);

	if (!FX_SUCCEEDED(BoundsResult))
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::GetAnimationBounds. FaceFX call <fxAnimationGetBounds> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(BoundsResult), *GetNameSafe(pAnimation));
	}

	FxResult DestroyResult = fxAnimationDestroy(&Animation, nullptr, nullptr);

	if (!FX_SUCCEEDED(DestroyResult))
	{
		UE_LOG(LogFaceFX, Error, TEXT("FaceFX::GetAnimationBounds. FaceFX call <fxAnimationDestroy> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(DestroyResult), *GetNameSafe(pAnimation));
	}

	return (FX_SUCCEEDED(BoundsResult) && FX_SUCCEEDED(DestroyResult));
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
		if (!GIsEditor)
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