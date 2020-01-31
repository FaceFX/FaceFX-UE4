/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2020 OC3 Entertainment, Inc. All rights reserved.
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

#pragma once

#include "FaceFXEditorConfig.generated.h"

/** Settings for the editor which are exposed to the project plugin config screen */
UCLASS(config=Editor, defaultconfig)
class FACEFXEDITOR_API UFaceFXEditorConfig : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Gets the config instance
	* @returns The config instance
	*/
	static const UFaceFXEditorConfig& Get()
	{
		const UFaceFXEditorConfig* Instance = GetDefault<UFaceFXEditorConfig>();
		check(Instance);
		return *Instance;
	}

	/**
	* Gets the path to the FaceFX plugin
	* @returns The path to the FaceFX folder
	*/
	const FString& GetFaceFXPluginFolder() const;

	/**
	* Gets the path to the FaceFX Studio installation. Configurable via engine ini file (section "ThirdParty.FaceFX", property "StudioPathAbsolute")
	* @returns The path to the installation
	*/
	inline const FString& GetFaceFXStudioPath() const
	{
		return StudioPath;
	}

	/**
	* Gets the indicator if the import shall search through all existing USoundWave assets and look for an asset that was generated with the linked sound source file per FaceFX Animation.
	* @returns True if enabled, else false
	*/
	inline bool IsImportLookupAudio() const
	{
		return bIsImportLookupAudio;
	}

	/**
	* Gets the indicator if the import shall search through all existing UFaceFXAnimation assets and look for an asset that was generated with the linked .ffxanim source file.
	* @returns True if enabled, else false
	*/
	inline bool IsImportLookupAnimation() const
	{
		return bIsImportLookupAnimation;
	}

	/**
	* Gets the indicator if the audio data (.wav files only) shall be automatically imported during the FaceFX Animation import process
	* @returns True if enabled, else false
	*/
	inline bool IsImportAudio() const
	{
		return bIsImportAudio;
	}

	/**
	* Gets the indicator if animations shall be imported during FaceFX actor import
	* @returns True if enabled, else false
	*/
	inline bool IsImportAnimationOnActorImport() const
	{
		return bIsImportAnimationOnActorImport;
	}

	/**
	* Gets the indicator if the editor shall show a warning toaster message when an UFaceFXAnimation is tried to get played on an
	* UFaceFXCharacter which FaceFX actor handle is incompatible with that animation
	* @returns True if enabled, else false
	*/
	inline bool IsShowToasterMessageOnIncompatibleAnim() const
	{
		return bShowToasterMessageOnIncompatibleAnim;
	}

private:

	/** The plugin folder path */
	mutable FString PluginFolderPath;

	/** The absolute path to the FaceFX Studio installation. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	FString StudioPath = "C:/Program Files/FaceFX/FaceFX Studio Professional 2018/facefx-studio.exe";

	/* Indicates if the audio data(.wav files only) should be automatically imported during the FaceFX Animation import process. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	bool bIsImportAudio = true;

	/* Indicates if the import should search through all existing USoundWave assets and look for an asset that was generated with the linked sound source
file(per FaceFX Animation).If such one is found it will be used instead of creating a new USoundWave asset for the.wav file.
Note: This might affect performance heavily when there are a lot of USoundWave assets. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	bool bIsImportLookupAudio = true;

	/* Indicates if animations should be imported during FaceFX actor import.If set to false only the FaceFX Actor asset will be imported or updated. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	bool bIsImportAnimationOnActorImport = true;

	/* Indicates if the import should search through all existing UFaceFXAnimation assets and look for an asset that was generated with the linked
.ffxanim source file.If such one is found it will be used instead of creating a new UFaceFXAnimation asset for the.ffxanim file.
Note: This might affect performance heavily when there are a lot of UFaceFXAnimation assets. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	bool bIsImportLookupAnimation = true;

	/* Indicates if the editor should show a warning toaster message when a UFaceFXAnimation is attempted to be played on a UFaceFXCharacter with an incompatible FaceFX actor handle. */
	UPROPERTY(config, EditAnywhere, Category = FaceFX)
	bool bShowToasterMessageOnIncompatibleAnim = true;
};
