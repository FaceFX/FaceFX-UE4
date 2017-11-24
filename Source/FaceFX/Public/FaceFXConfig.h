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

#pragma once

#include "Modules/ModuleVersion.h"
#include "facefx-runtime-1.3.0/facefx/facefx.h"

// The number of total FaceFX channels. One channel per animation we want to be
// able to play at once per FaceFXCharacter. Default Value: 1
#define FACEFX_CHANNELS 1

// Indicator if the FaceFX Actor assets shall maintain links to any imported
// FaceFX Animation asset. Default Value: 1
// When true those linked assets will always be cooked and playback can be
// triggered using ID Strings (and assets). This imply some additional memory
// overhead and cooking of possibly unused animations
// When false playback can only be triggered using an asset reference.
// Maintaining such assets for cooking is the responsibility of the developer
#define FACEFX_USEANIMATIONLINKAGE 1

// Indicator if imported .ffxanim will be deleted. Default Value: 1
// When true those asset will be deleted after import and considered as
// up-to-date when the asset source file does exist during reimport.
// When false those files are not deleted and are considered always newer than
// asset. Reimport will always be performed
#define FACEFX_DELETE_IMPORTED_ANIM 1

// Indicator if the whole .ffxc folder shall be deleted when there is no
// .ffxanim left inside after loading compiled data. Default Value: 1
#define FACEFX_DELETE_EMPTY_COMPILATION_FOLDER 1

// The root namespace for any ini file entry
#define FACEFX_CONFIG_NS TEXT("FaceFX")

// File extension: FaceFX Studio asset
#define FACEFX_FILEEXT_FACEFX TEXT(".facefx")

// File extension: Animation data
#define FACEFX_FILEEXT_ANIM TEXT(".ffxanim")

// File extension search filter: Animation data
#define FACEFX_FILEEXT_ANIMSEARCH TEXT("*.ffxanim")

// File extension: Bone map
#define FACEFX_FILEEXT_BONES TEXT(".ffxbones")

// File extension: FaceFX Actor
#define FACEFX_FILEEXT_ACTOR TEXT(".ffxactor")

// File extension: Animation Ids
#define FACEFX_FILEEXT_ANIMID TEXT(".ffxids")

// File extension: Audio map
#define FACEFX_FILEEXT_AUDIO TEXT(".ffxamap")

// File dialog filter for FaceFX Actor source assets
#define FACEFX_FILEFILTER_ASSET_ACTOR TEXT("FaceFX Asset (*.facefx)|*.facefx;")

// File dialog filter for FaceFX Animation source assets
#define FACEFX_FILEFILTER_ASSET_ANIM                                           \
  TEXT("FaceFX Animation Asset (*.ffxanim)|*.ffxanim;")

// The different platforms that are supported by the FaceFX integration. This
// must correspond to the FaceFX compiler platform plugin export settings.
// When enabling support for a platform all assets must be reimported in order
// to have the new platform data available during cooking
// Also remember to adjust the WhitelistPlatforms property within FaceFX.uplugin
// accordingly to enable FaceFX on these target platforms
// The module must also link to the PS4, XBoxOne, and Switch libs. To enable uncomment
// the place within FaceFXLib.Build.cs.
// Playstation 4
#define FACEFX_SUPPORT_PS4 0
// XBox One
#define FACEFX_SUPPORT_XBONE 0
// Nintendo Switch
#define FACEFX_SUPPORT_SWITCH 0

// Support for sequencer which was added with UE 4.12.
#if ENGINE_MAJOR_VERSION < 4 && ENGINE_MINOR_VERSION < 13
#error                                                                         \
    "FaceFX Sequencer support requires Unreal Engine 4.12 or higher. Please update your engine or use a previous version of the plugin.";
#endif


#if WITH_EDITOR

/** A layer to access the FaceFX ini config values */
struct FACEFX_API FFaceFXConfig
{
	/** Gets the singleton instance */
	static const FFaceFXConfig& Get();

	/**
	* Gets the path to the FaceFX plugin
	* @returns The path to the FaceFX folder
	*/
	static const FString& GetFaceFXPluginFolder();

	/**
	* Gets the path to the FaceFX.ini file
	* @returns The path to the ini
	*/
	static const FString& GetFaceFXIni();

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

	/**
	* Gets the indicator if the active audio system is using sound wave assets
	* @returns True if sound wave assets are used, else false
	*/
	static bool IsAudioUsingSoundWaveAssets();

private:

	FFaceFXConfig();

	/** Path to FaceFX Studio */
	FString StudioPath;

	/** Indicator if audio assets shall be looked up during import */
	bool bIsImportLookupAudio;

	/** Indicator if animation assets shall be looked up during import */
	bool bIsImportLookupAnimation;

	/** Indicator if audio shall be imported during FaceFX import process */
	bool bIsImportAudio;

	/** Indicator if animations shall be imported during FaceFX actor import */
	bool bIsImportAnimationOnActorImport;

	/** Indicator if the editor shall show a warning toaster message when an UFaceFXAnimation is tried to get played on */
	bool bShowToasterMessageOnIncompatibleAnim;
};

#endif //WITH_EDITOR
