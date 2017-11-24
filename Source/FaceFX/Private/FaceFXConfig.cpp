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

#include "FaceFX.h"
#include "FaceFXConfig.h"

#if WITH_EDITOR
#include "ConfigCacheIni.h"
#include "Audio/FaceFXAudio.h"

const FFaceFXConfig& FFaceFXConfig::Get()
{
	static FFaceFXConfig Instance;
	return Instance;
}

const FString& FFaceFXConfig::GetFaceFXPluginFolder()
{
	static FString FolderPath;
	if (FolderPath.IsEmpty())
	{
		FolderPath = FPaths::EnginePluginsDir() / TEXT("Runtime/FaceFX");
		if (!FPaths::DirectoryExists(FolderPath))
		{
			FolderPath = FPaths::ProjectPluginsDir() / TEXT("FaceFX");
		}
	}
	return FolderPath;
}

const FString& FFaceFXConfig::GetFaceFXIni()
{
	static FString IniPath = GetFaceFXPluginFolder() / TEXT("Config/FaceFX.ini");
	return IniPath;
}

FFaceFXConfig::FFaceFXConfig() : bIsImportLookupAudio(false), bIsImportLookupAnimation(false), bIsImportAudio(false), bIsImportAnimationOnActorImport(false), bShowToasterMessageOnIncompatibleAnim(false)
{
	const FString IniFile = GetFaceFXIni();

	{
		//FaceFX Studio Path
		if (!GConfig->GetString(FACEFX_CONFIG_NS, TEXT("StudioPathAbsolute"), StudioPath, IniFile))
		{
			//fallback to default
			StudioPath = TEXT("C:/Program Files (x86)/FaceFX/FaceFX Studio Professional 2017/facefx-studio.exe");
		}
	}

	GConfig->GetBool(FACEFX_CONFIG_NS, TEXT("IsImportLookupAudio"), bIsImportLookupAudio, IniFile);
	GConfig->GetBool(FACEFX_CONFIG_NS, TEXT("IsImportLookupAnimation"), bIsImportLookupAnimation, IniFile);
	GConfig->GetBool(FACEFX_CONFIG_NS, TEXT("IsImportAudio"), bIsImportAudio, IniFile);
	GConfig->GetBool(FACEFX_CONFIG_NS, TEXT("IsImportAnimationOnActorImport"), bIsImportAnimationOnActorImport, IniFile);
	GConfig->GetBool(FACEFX_CONFIG_NS, TEXT("ShowToasterMessageOnIncompatibleAnim"), bShowToasterMessageOnIncompatibleAnim, IniFile);
}

bool FFaceFXConfig::IsAudioUsingSoundWaveAssets()
{
	return FFaceFXAudio::IsUsingSoundWaveAssets();
}

#endif //WITH_EDITOR
