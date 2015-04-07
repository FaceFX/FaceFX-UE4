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

#pragma once

#include "FaceFXData.h"

/** Editor specific FaceFX functions */
struct FACEFXEDITOR_API FFaceFXEditorTools
{
	/**
	* Gets the path to the FaceFX Studio installation. Configurable via engine ini file (section "ThirdParty.FaceFX", property "StudioPathAbsolute")
	* @returns The path to the installation
	*/
	static const FString& GetFaceFXStudioPath();

	/**
	* Gets the path to the FaceFX compiler. Configurable via engine ini file (section "ThirdParty.FaceFX", property "CompilerPathRelative")
	* @returns The path to the FaceFX compiler
	*/
	static const FString& GetFaceFXCompilerPath();

	/**
	* Checks if FaceFX studio is installed within the standard installation path
	* @returns True if installed, else false
	*/
	static inline bool IsFaceFXStudioInstalled()
	{
		return FPaths::FileExists(GetFaceFXStudioPath());
	}

	/**
	* Gets the indicator if the compiler is installed
	* @returns True if installed else false
	*/
	static inline bool IsFaceFXCompilerInstalled()
	{
		return FPaths::FileExists(GetFaceFXCompilerPath());
	}

	/**
	* Opens the given FaceFX asset within FaceFX studio in case this is installed locally
	* @param Asset The asset to open
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if open succeeded, else false
	*/
	static bool OpenFaceFXStudio(class UFaceFXActor* Asset, FString* OutErrorMessage = nullptr);

	/**
	* Reimports a given FaceFX asset
	* @param Asset The target asset to reimport
	* @param OutErrorMessage The optional error message in case it failed
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ReImportFaceFXAsset(class UFaceFXAsset* Asset, FString* OutErrorMessage = nullptr, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Imports a given FaceFX asset
	* @param Asset The target asset to import into
	* @param AssetPath The path to the .facefx asset file
	* @param OutErrorMessage The optional error message in case it failed
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ImportFaceFXAsset(class UFaceFXAsset* Asset, const FString& AssetPath, FString* OutErrorMessage = nullptr, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Gets all animation groups found within a given FaceFX compilation folder and platform specific sub folder
	* @param Folder The compilation folder to load from
	* @param Platform The target platform to use the sub folder from
	* @param OutGroups The resulting animation groups
	* @returns True if succeeded, else false
	*/
	inline static bool GetAnimationGroupsInFolder(const FString& Folder, EFaceFXTargetPlatform::Type Platform, TArray<FString>& OutGroups)
	{
		return GetAnimationGroupsInFolder(GetCompilationFolder(Folder, Platform), OutGroups);
	}

	/**
	* Gets all animation groups found within a given FaceFX compilation folder
	* @param Folder The compilation folder to load from
	* @param OutGroups The resulting animation groups
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationGroupsInFolder(const FString& Folder, TArray<FString>& OutGroups);

	/**
	* Gets the FaceFX temp folder
	* @returns The temp folder
	*/
	inline static FString GetTempFolder()
	{
		return FPaths::GameSavedDir() / TEXT("TmpFaceFX");
	}

	/**
	* Gets the compilation sub folder for a given folder
	* @param Folder The base folder to generate the target folder for
	* @param Platform The target platform
	* @returns The target folder
	*/
	static inline FString GetCompilationFolder(const FString& Folder, EFaceFXTargetPlatform::Type Platform)
	{
		switch(Platform)
		{
		case EFaceFXTargetPlatform::PC: return Folder / TEXT("x86");
		case EFaceFXTargetPlatform::PS4: return Folder / TEXT("ps4");
		case EFaceFXTargetPlatform::XBoxOne: return Folder / TEXT("xboxone");
		default: checkf(false, TEXT("Unknown target platform type"));
		}
		return TEXT("");
	}

	/**
	* Gets the timeout duration set in seconds. Default is 120sec. Configurable via engine ini file (section "ThirdParty.FaceFX", property "CompilationTimeoutSec")
	* @returns The current duration
	*/
	static float GetCompilationTimeoutDuration();

	/**
	* Creates a new anim set asset
	* @param CompilationFolder The folder to load the data from
	* @param AnimGroup The animation group folder to load the data from
	* @param PackageName The name of the package
	* @param FaceFXActor The FaceFX actor asset that shall be linked to that new asset
	* @param AssetTools The asset tools instance to use
	* @param Factory The factory to use. Keep at nullptr to directly create an instance
	*/
	static class UFaceFXAnimSet* CreateAnimSetAsset(const FString& CompilationFolder, const FString& AnimGroup, const FString& PackageName, class UFaceFXActor* FaceFXActor, class IAssetTools& AssetTools, class UFactory* Factory = nullptr);

	/**
	* Performs steps to store an asset physically and in source control
	* @param Package The package to save
	*/
	static void SavePackage(UPackage* Package);

	/** 
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAsset* Asset, const FString& Folder, FString* OutErrorMessage = nullptr);

	/** 
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXActor* Asset, const FString& Folder, FString* OutErrorMessage = nullptr);

	/** 
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAnimSet* Asset, const FString& Folder, FString* OutErrorMessage = nullptr);

	/** 
	* Initializes the asset from a .facefx asset file
	* @param Asset The target asset
	* @param File The path to the .facefx asset file
	* @param ErrorMessage The optional error message in case it failed
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if succeeded, else false
	*/
	static bool InitializeFromFile(class UFaceFXAsset* Asset, const FString& File, FString* OutErrorMessage, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate());

private:

	FFaceFXEditorTools(){}

	/** 
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder. Assigns a new group before doing so
	* @param asset The target asset
	* @param group The new group
	* @param folder The path to the platform folder to load the compiled assets from
	* @param outErrorMessage The optional error message in case it failed
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAnimSet* asset, const FName& group, const FString& folder, FString* outErrorMessage = nullptr);

	/** 
	* Loads all assets from a given folder into the given target struct
	* @param Folder The path to the folder to load the compiled assets from
	* @param AssetName The name of the assets to load without any extension
	* @param TargetData The target struct to load into
	* @returns True if succeeded, else false
	*/
	static bool LoadCompiledData(const FString& Folder, const FString& AssetName, FFaceFXActorData& TargetData);

	/** 
	* Loads all assets from a given folder into the given target struct
	* @param Folder The path to the folder to load the compiled assets from
	* @param TargetData The target struct to load into
	* @returns True if succeeded, else false
	*/
	static bool LoadCompiledData(const FString& Folder, FFaceFXAnimSetData& TargetData);

	/**
	* Compiles the given .facefx asset file within a new temporary folder
	* @param AssetPath The file path
	* @param OutCompiledFile The temporary file which will be in the temporary folder that also contain all the compiled assets
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if open succeeded, else false
	*/
	static bool CompileAssetToTempFolder(const FString& AssetPath, FString& OutCompiledFile, FString* OutErrorMessage = nullptr);
};
