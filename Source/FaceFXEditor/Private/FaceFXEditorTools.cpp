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

#include "FaceFXEditorTools.h"
#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "Audio/FaceFXAudio.h"
#include "EditorStyleSet.h"
#include "Include/Slate/FaceFXComboChoiceWidget.h"
#include "FaceFXEditorConfig.h"
#include "AssetToolsModule.h"
#include "ObjectTools.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"
#include "ISourceControlOperation.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Sound/SoundWave.h"
#include "Editor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FaceFX"

uint32 FFaceFXEditorTools::AssetCategory = 0;

/**
* Makes a given string conform to be used as an asset identifier
* @param Name The name to make conform
* @returns The conform asset string
*/
FString MakeAssetConform(const FString& Name)
{
	return Name.Replace(TEXT(" "), TEXT(""), ESearchCase::CaseSensitive).Replace(TEXT("."), TEXT("_"), ESearchCase::CaseSensitive);
}

/**
* Gets the indicator if the given relative folder is empty or contains any file
* @param RelativeFolder The folder to check
* @returns True if empty, else false
*/
bool IsFolderEmpty(const FString& RelativeFolder)
{
	//check if the temp folder is empty, if so delete
	struct FDirEmptyCheck : public IPlatformFile::FDirectoryVisitor
	{
		bool bIsEmpty;
		FDirEmptyCheck() : bIsEmpty(true) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			bIsEmpty = false;
			return false;
		}
	} DirIterator;

	const FString Folder = FPaths::ConvertRelativePathToFull(RelativeFolder);
	IFileManager::Get().IterateDirectory(*Folder, DirIterator);
	return DirIterator.bIsEmpty;
}

/**
* Gets the filename for an animation asset
* @param Folder The compilation folder
* @param Group The animation group id
* @param AnimationId The animation id
*/
FString GetAnimAssetFileName(const FString& Folder, const FString& Group, const FString& AnimationId)
{
	return Folder / Group / (AnimationId + FACEFX_FILEEXT_ANIM);
}

/**
* Finds a UFaceFXAnim asset that is linked to the same source file
* @returns The asset that links to the same source file or nullptr if not found
*/
UFaceFXAnim* FindAnimationAsset(const FString& SourceAssetFolder, const FString& SourceAssetFile, const FName& AnimGroup, const FName& AnimName)
{
	if(SourceAssetFolder.IsEmpty() || SourceAssetFile.IsEmpty() || AnimGroup.IsNone() || AnimName.IsNone())
	{
		//sanity check
		return nullptr;
	}

	IFileManager& FileManager = IFileManager::Get();

	FString SourceAssetFolderAbs;

	TArray<FAssetData> Assets;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	if(AssetRegistry.GetAssetsByClass(UFaceFXAnim::StaticClass()->GetFName(), Assets))
	{
		for(const FAssetData& AssetData : Assets)
		{
			UFaceFXAnim* Asset = Cast<UFaceFXAnim>(AssetData.GetAsset());

			if(!Asset)
			{
				//synchronously load sound asset
				Asset = Cast<UFaceFXAnim>(StaticLoadObject(UFaceFXAnim::StaticClass(), nullptr, *AssetData.ToSoftObjectPath().ToString()));
			}

			checkf(Asset, TEXT("Internal Error. UFaceFXAnim asset not loaded. %s"), *AssetData.ToSoftObjectPath().ToString());

			//check if the anim refers to the same group/id and belong to the same source .facefx
			const FFaceFXAnimId& AssetAnimId = Asset->GetId();
			if(AssetAnimId.Group == AnimGroup && AssetAnimId.Name == AnimName && SourceAssetFile.Equals(Asset->GetAssetName(), ESearchCase::IgnoreCase))
			{
				if(SourceAssetFolderAbs.IsEmpty())
				{
					//lazy conversion
					SourceAssetFolderAbs = FileManager.ConvertToAbsolutePathForExternalAppForRead(*SourceAssetFolder);
				}

				const FString AssetFolderAbs = FileManager.ConvertToAbsolutePathForExternalAppForRead(*Asset->GetAssetFolder());
				if(AssetFolderAbs.Equals(SourceAssetFolderAbs, ESearchCase::IgnoreCase))
				{
					return Asset;
				}
			}
		}
	}

	return nullptr;
}

/** A single audio map entry*/
struct FFaceFXAudioMapEntry
{
	/** The animation group*/
	FString Group;

	/** The animation id */
	FString AnimationId;

	/** The relative audio path */
	FString AudioPath;
};

/**
* Loads an audio map file
* @param Asset The asset to load the audio map for
* @param Folder The compilation root folder
* @param OutResult The resulting audio map data
* @param The message container to push error messages into
* @returns True if file was loaded and parsed, else false if any file access error occurred
*/
bool LoadAudioMapData(UFaceFXAsset* Asset, const FString& Folder, TArray<FFaceFXAudioMapEntry>& OutResult, FFaceFXImportResult& OutResultMessages)
{
	//link with audio data base on the PC mapping (all platforms use the same audio data)
	const FString AudioMapFile = Folder / (Asset->GetAssetName() + FACEFX_FILEEXT_AUDIO);
	if(FPaths::FileExists(AudioMapFile))
	{
		TArray<FString> Lines;
		if(FFileHelper::LoadANSITextFileToStrings(*AudioMapFile, nullptr, Lines))
		{
			for(const FString& Line : Lines)
			{
				if(Line.IsEmpty())
				{
					continue;
				}

				FFaceFXAudioMapEntry NewEntry;

				//Format: <Group>/<AnimId>|<Relative Path>
				FString GroupSRight;
				if(Line.Split(TEXT("/"), &NewEntry.Group, &GroupSRight) && GroupSRight.Split(TEXT("|"), &NewEntry.AnimationId, &NewEntry.AudioPath))
				{
					if(NewEntry.Group.Contains(TEXT("/")))
					{
						UE_LOG(LogFaceFX, Warning, TEXT("FFaceFXEditorTools::LoadAudioMapData. The audio mapping file contains an invalid '/' character in the animation group name. Entry will be ignored. File: %s. Asset: %s. Line: %s"), *AudioMapFile, *GetNameSafe(Asset), *Line);
						OutResultMessages.AddModifyWarning(FText::Format(LOCTEXT("LoadAudioMapDataBadCharGroup", "The audio mapping file contains an invalid '/' character in the animation group name. Entry will be ignored. File: {0}, Line: {1}"), FText::FromString(AudioMapFile), FText::FromString(Line)), Asset);
						continue;
					}

					if(NewEntry.AnimationId.Contains(TEXT("/")))
					{
						UE_LOG(LogFaceFX, Warning, TEXT("FFaceFXEditorTools::LoadAudioMapData. The audio mapping file contains an invalid '/' character in the animation name. Entry will be ignored. File: %s. Asset: %s. Line: %s"), *AudioMapFile, *GetNameSafe(Asset), *Line);
                        OutResultMessages.AddModifyWarning(FText::Format(LOCTEXT("LoadAudioMapDataBadCharAnimId", "The audio mapping file contains an invalid '/' character in the animation name. Entry will be ignored. File: {0}, Line: {1}"), FText::FromString(AudioMapFile), FText::FromString(Line)), Asset);
						continue;
					}

					//entry found
					if(NewEntry.AudioPath.Equals(TEXT("-")))
					{
						//consider "-" as empty
						NewEntry.AudioPath = TEXT("");
					}

					OutResult.Add(NewEntry);
				}
			}
			return true;
		}
		else
		{
            OutResultMessages.AddModifyWarning(FText::Format(LOCTEXT("LoadAudioMapDataLoadFailed", "The audio mapping file could not be loaded. File: {0}"), FText::FromString(AudioMapFile)), Asset);
		}
	}
	else
	{
        OutResultMessages.AddModifyWarning(FText::Format(LOCTEXT("LoadAudioMapDataMissingFile", "The audio mapping file does not exist. File: {0}"), FText::FromString(AudioMapFile)), Asset);
	}
	return false;
}

/**
* Gets the indicator if a given asset is listed inside an audio map data
* @param Asset The asset to look for
* @param AudioMapData The audio map data to look inside
* @returns True if found, else false
*/
bool IsAnimationExistInAudioMap(const UFaceFXAnim* Asset, const TArray<FFaceFXAudioMapEntry>& AudioMapData)
{
	check(Asset);
	for(const FFaceFXAudioMapEntry& AudioMapEntry : AudioMapData)
	{
		if(AudioMapEntry.AnimationId.Equals(Asset->GetName().ToString(), ESearchCase::IgnoreCase) &&
			AudioMapEntry.Group.Equals(Asset->GetGroup().ToString(), ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

#if FACEFX_DELETE_EMPTY_COMPILATION_FOLDER
/**
* Deletes the whole .ffxc folder when there is no animation file left inside
* @param Folder The folder to delete
* @returns True if there was no animation file and the folder got deleted, else false
*/
bool CleanupCompilationFolder(const FString& Folder)
{
	//check if any .ffxanim file is left
	TArray<FString> Filenames;
	IFileManager::Get().FindFilesRecursive(Filenames, *Folder, FACEFX_FILEEXT_ANIMSEARCH, true, false, false);

	if(Filenames.Num() == 0)
	{
		return IFileManager::Get().DeleteDirectory(*Folder, false, true);
	}

	return false;
}
#endif //FACEFX_DELETE_EMPTY_COMPILATION_FOLDER

bool FFaceFXImportActionResult::Rollback()
{
	//only creations can be rolled back as a whole for now
	if(CanRollback())
	{
		//first check if the action owns a contextual asset which can be deleted (audio assets)
		UObject* CreatedAsset = Asset.Get();

		if(!CreatedAsset)
		{
			//if such one is not set -> delete the imported asset instead
			CreatedAsset = ImportAsset.Get();
		}

		if(CreatedAsset)
		{
			FFaceFXEditorTools::DeleteAsset(CreatedAsset);
		}

		return true;
	}

	return false;
}

bool FFaceFXEditorTools::IsFaceFXStudioInstalled()
{
	return FPaths::FileExists(UFaceFXEditorConfig::Get().GetFaceFXStudioPath());
}

bool FFaceFXEditorTools::OpenFaceFXStudio(UFaceFXActor* Asset, FString* OutErrorMessage)
{
	if(!Asset)
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	const FString StudioPath = UFaceFXEditorConfig::Get().GetFaceFXStudioPath();
	if(!FPaths::FileExists(StudioPath))
	{
		//asset does not exist
		if(OutErrorMessage)
		{
			*OutErrorMessage = FText::Format(LOCTEXT("StartFailStudioMissing", "FaceFX start failed: FaceFX Studio was not found: '{0}'."), FText::FromString(StudioPath)).ToString();
		}
		return false;
	}

	if(!Asset->IsAssetPathSet())
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("StartFailNotSet", "FaceFX start failed: Asset source is not set. Check asset and assign a new source.").ToString();
		}
		return false;
	}

	const FString AssetPath = FPaths::ConvertRelativePathToFull(Asset->GetAssetPath());
	if(!FPaths::FileExists(AssetPath))
	{
		//asset does not exist
		if(OutErrorMessage)
		{
			*OutErrorMessage = FText::Format(LOCTEXT("StartFailMissing", "FaceFX start failed: Asset source file does not exist: '{0}'."), FText::FromString(AssetPath)).ToString();
		}
		return false;
	}

	const FString Args = TEXT("\"") + AssetPath + TEXT("\"");

	//create startup script file
	const FProcHandle process = FPlatformProcess::CreateProc(*StudioPath, *Args, true, true, false, nullptr, 0, nullptr, nullptr);
	if(!process.IsValid())
	{
		//process creation failed
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("StartFail", "FaceFX start failed: Creating the process failed.").ToString();
		}
		return false;
	}

	return true;
}

bool FFaceFXEditorTools::ReImportFaceFXAsset(UFaceFXAsset* Asset, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback)
{
	if(!Asset)
	{
		OutResultMessages.AddModifyError(LOCTEXT("MissingAsset", "Target asset missing."), Asset);
		return false;
	}

	return ImportFaceFXAsset(Asset, Asset->GetAssetPath(), OutResultMessages, BeforeDeletionCallback);
}

bool FFaceFXEditorTools::ImportFaceFXAsset(UFaceFXAsset* Asset, const FString& AssetPath, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback)
{
	if(!Asset)
	{
		OutResultMessages.AddModifyError(LOCTEXT("MissingAsset", "Target asset missing."), Asset);
		return false;
	}

	const FString CompilationFolder = GetCompilationFolder(AssetPath);
	const bool ImportResult = LoadFromCompilationFolder(Asset, CompilationFolder, OutResultMessages);

	BeforeDeletionCallback.ExecuteIfBound(Asset, CompilationFolder, ImportResult, OutResultMessages);

	if(ImportResult)
	{
		SavePackage(Asset->GetOutermost());
		OutResultMessages.AddModifySuccess(LOCTEXT("ImportFaceFXAssetSuccess", "(Re)Importing FaceFX asset succeeded."), Asset);
	}
	else
	{
		OutResultMessages.AddModifyError(LOCTEXT("ImportFaceFXAssetFail", "(Re)Importing FaceFX asset failed."), Asset);
	}

#if FACEFX_DELETE_EMPTY_COMPILATION_FOLDER
	CleanupCompilationFolder(CompilationFolder);
#endif //FACEFX_DELETE_EMPTY_COMPILATION_FOLDER

	//inform FaceFX characters about updated asset
	UFaceFXCharacter::OnAssetChanged.Broadcast(Asset);

	return ImportResult;
}

bool FFaceFXEditorTools::LoadCompiledActorData(UFaceFXActor* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	const FString& AssetName = Asset->AssetName;

	bool allPresent = true;

	const FString AssetPathActor = FPaths::Combine(*Folder, *(AssetName + FACEFX_FILEEXT_ACTOR));
	if(!FPaths::FileExists(AssetPathActor))
	{
		OutResultMessages.AddModifyError(FText::Format(LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. File doesn't exist: {0}"),
		                                               FText::FromString(AssetPathActor)), Asset);
		allPresent = false;
	}

	//bones are optional, so don't include the file in the checks here.

	const FString AssetPathIds = FPaths::Combine(*Folder, *(AssetName + FACEFX_FILEEXT_ANIMID));
	if(!FPaths::FileExists(AssetPathIds))
	{
		OutResultMessages.AddModifyError(FText::Format(LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. File doesn't exist: {0}"),
		                                               FText::FromString(AssetPathIds)), Asset);
		allPresent = false;
	}

	if(!allPresent)
	{
		return false;
	}

	//load the data (we know all the files exist at this point)

	FFaceFXActorData& Data = Asset->GetData();

	bool DataPreviouslyHadBones = Data.BonesRawData.Num() > 0;

	Asset->Reset();

	Data.Reset();

	FFileHelper::LoadFileToArray(Data.ActorRawData, *AssetPathActor);

	//bones are optional
	const FString AssetPathBones = FPaths::Combine(*Folder, *(AssetName + FACEFX_FILEEXT_BONES));
	if(FPaths::FileExists(AssetPathBones))
	{
		FFileHelper::LoadFileToArray(Data.BonesRawData, *AssetPathBones);
	}
	else
	{
	    //if the data previously had bones but now it doesn't, issue a warning
	    if(DataPreviouslyHadBones)
	    {
		OutResultMessages.AddModifyWarning(FText::Format(LOCTEXT("LoadingCompiledAsset", "The asset previously contained bones data, but now it does not (the file doesn't exist: {0})"),
		                                                 FText::FromString(AssetPathBones)), Asset);
	    }
	}

	TArray<FString> Lines;
	if (FFileHelper::LoadANSITextFileToStrings(*AssetPathIds, nullptr, Lines))
	{
		for (const FString& Line : Lines)
		{
			FString Ids, Name;
			if (Line.Split(":", &Ids, &Name))
			{
				if (Ids.Len() <= 16)
				{
					//accept only max 8 hex values as input
					uint64 Id = 0;
					while (Ids.Len() > 0)
					{
						Id = Id << 8;
						Id |= FParse::HexNumber(*(Ids.Left(2)));
						Ids = Ids.RightChop(2);
					}
					Data.Ids.Add(FFaceFXIdData(Id, FName(*Name)));
				}
			}
		}
	}

	return true;
}

bool FFaceFXEditorTools::GetAnimationGroupsInFolder(const FString& Folder, TArray<FString>* OutGroups, TArray<FString>* OutAnimGroupIds)
{
	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//folder not exist
		return false;
	}

	//load all sub folders
	TArray<FString> Directories;
	IFileManager::Get().FindFiles(Directories, *(Folder / TEXT("*")), false, true);

	//load all animations within the sub folders
	for(const FString& Dir : Directories)
	{
		TArray<FString> Filenames;
		IFileManager::Get().FindFiles(Filenames, *(Folder / Dir / FACEFX_FILEEXT_ANIMSEARCH), true, false);

		if(Filenames.Num() > 0)
		{
			if(OutGroups)
			{
				OutGroups->Add(FPaths::GetCleanFilename(Dir));
			}

			if(OutAnimGroupIds)
			{
				for(const FString& Filename : Filenames)
				{
					OutAnimGroupIds->Add(Dir + TEXT(".") + FPaths::GetBaseFilename(Filename));
				}
			}
		}
	}

	return true;
}

bool FFaceFXEditorTools::ReimportOrCreateAnimAssets(const FString& CompilationFolder, const FString& AnimGroup, const FString& PackageName, UFaceFXActor* FaceFXActor, IAssetTools& AssetTools, FFaceFXImportResult& OutResultMessages, UFactory* Factory, TArray<UFaceFXAnim*>* OutResult)
{
	TArray<FString> Filenames;
	IFileManager::Get().FindFiles(Filenames, *(CompilationFolder / AnimGroup / FACEFX_FILEEXT_ANIMSEARCH), true, false);

	bool Result = true;

	for(const FString& Filename : Filenames)
	{
		if(UFaceFXAnim* Asset = ReimportOrCreateAnimAsset(CompilationFolder, AnimGroup, FPaths::GetCleanFilename(Filename), PackageName, FaceFXActor, AssetTools, OutResultMessages, Factory))
		{
			if(OutResult)
			{
				OutResult->Add(Asset);
			}
		}
		else
		{
			Result = false;
		}
	}

	return Result;
}

UFaceFXAnim* FFaceFXEditorTools::ReimportOrCreateAnimAsset(const FString& CompilationFolder, const FString& AnimGroup, const FString& AnimFile, const FString& PackageName, UFaceFXActor* FaceFXActor, IAssetTools& AssetTools, FFaceFXImportResult& OutResultMessages, UFactory* Factory)
{
	if(!FaceFXActor)
	{
		return nullptr;
	}

	const FString AnimId = FPaths::GetBaseFilename(AnimFile);
	const FName AnimGroupName = FName(*AnimGroup);
	const FName AnimIdName = FName(*AnimId);

	//First try to find an existing Animation Asset that links to the same animation file
	const FString Filename = GetAnimAssetFileName(CompilationFolder, AnimGroup, AnimFile);

	if(UFaceFXEditorConfig::Get().IsImportLookupAnimation())
	{
		//Try to find an existing UFaceFXAnim asset that is using the same source .ffxanim file which we could reuse
		if(UFaceFXAnim* ExistingAnim = FindAnimationAsset(FaceFXActor->GetAssetFolder(), FaceFXActor->GetAssetName(), AnimGroupName, AnimIdName))
		{
			if(LoadFromCompilationFolder(ExistingAnim, AnimGroupName, AnimIdName, CompilationFolder, OutResultMessages))
			{
#if FACEFX_USEANIMATIONLINKAGE
				//link to the new asset
				FaceFXActor->LinkTo(ExistingAnim);
				OutResultMessages.AddModifySuccess(LOCTEXT("CreateAssetAnimExistSuccess", "Reimported already existing animation Asset and linked to Import Triggering FaceFX Actor Asset."), ExistingAnim);
#else
				OutResultMessages.AddModifySuccess(LOCTEXT("CreateAssetAnimExistSuccess", "Reimported already existing animation Asset."), ExistingAnim);
#endif //FACEFX_USEANIMATIONLINKAGE

				SavePackage(ExistingAnim->GetOutermost());

				//inform FaceFX characters about updated asset
				UFaceFXCharacter::OnAssetChanged.Broadcast(ExistingAnim);

				return ExistingAnim;
			}
			else
			{
				OutResultMessages.AddModifyError(LOCTEXT("CreateAssetAnimExistFailed", "Reimport on Already existing animation Asset failed."), ExistingAnim);
				return nullptr;
			}
		}
	}

	//create new asset for this group
	FString NewAssetName, NewPackageName;
	AssetTools.CreateUniqueAssetName(PackageName, TEXT("_") + MakeAssetConform(AnimGroup) + TEXT("_") + MakeAssetConform(AnimId), NewPackageName, NewAssetName);

	//FAssetTools::CreateAsset is concatenating the /<assetname> of the packagepath itself, hence we remove it
	NewPackageName.RemoveFromEnd(TEXT("/") + NewAssetName);

	if(UFaceFXAnim* NewAsset = Cast<UFaceFXAnim>(AssetTools.CreateAsset(NewAssetName, NewPackageName, UFaceFXAnim::StaticClass(), Factory)))
	{
		//copy asset sources
		NewAsset->SetSources(FaceFXActor->GetAssetName(), FaceFXActor->GetAssetFolder());

		if(LoadFromCompilationFolder(NewAsset, AnimGroupName, AnimIdName, CompilationFolder, OutResultMessages))
		{
#if FACEFX_USEANIMATIONLINKAGE
			//link to the new asset
			FaceFXActor->LinkTo(NewAsset);
			OutResultMessages.AddCreateSuccess(LOCTEXT("CreateAssetAnimSuccess", "Animation Asset successfully created and linked to Import Triggering FaceFX Actor Asset."), NewAsset);
#else
			OutResultMessages.AddCreateSuccess(LOCTEXT("CreateAssetAnimSuccess", "Animation Asset successfully created."), NewAsset);
#endif //FACEFX_USEANIMATIONLINKAGE

			SavePackage(NewAsset->GetOutermost());

			//inform FaceFX characters about updated asset
			UFaceFXCharacter::OnAssetChanged.Broadcast(NewAsset);

			return NewAsset;
		}
		else
		{
			OutResultMessages.AddCreateError(LOCTEXT("CreateAssetAnimFailed", "Failed to create and load Animation Asset."), NewAsset);

			//delete asset again
			FFaceFXEditorTools::DeleteAsset(NewAsset);
		}
	}
	else
	{
		OutResultMessages.AddCreateError(LOCTEXT("CreateAssetAnimCreationFailed", "AssetTool Asset creation failed."), NewAsset);
	}

	return nullptr;
}

void FFaceFXEditorTools::SavePackage(UPackage* Package, bool addToSc)
{
	checkf(Package, TEXT("Missing package"));

	Package->MarkPackageDirty();
	Package->PostEditChange();

	const FString PackagePath = Package->GetPathName();
	const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	GEditor->SavePackage(Package, nullptr, RF_Standalone, *Filename, GWarn);

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if (addToSc && ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable() )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::ForceUpdate);
		if (!SourceControlState->IsCheckedOut() && !SourceControlState->IsAdded())
		{
			TArray<FString> Files;
			Files.Add(FPaths::ConvertRelativePathToFull(Filename));
			SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), Files);
		}
	}
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAsset* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	checkf(Asset, TEXT("Missing asset"));

	if(auto ActorAsset = Cast<UFaceFXActor>(Asset))
	{
		return LoadFromCompilationFolder(ActorAsset, Folder, OutResultMessages);
	}

	if(auto AnimAsset = Cast<UFaceFXAnim>(Asset))
	{
		return LoadFromCompilationFolder(AnimAsset, Folder, OutResultMessages);
	}

	checkf(false, TEXT("Unknown asset type"));

	return false;
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXActor* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	checkf(Asset, TEXT("Missing asset"));

	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//file not exist
		OutResultMessages.AddModifyError(FText::Format(LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: {0}"), FText::FromString(Folder)), Asset);

		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not exist: %s"), *Folder);
		return false;
	}

#if FACEFX_USEANIMATIONLINKAGE
	if(Asset->Animations.Num() > 0)
	{
		//check if linked animations are still part of the .ffxamap file. If not they were deleted and animations are stale
		TArray<FFaceFXAudioMapEntry> AudioMapData;
		if(LoadAudioMapData(Asset, Folder, AudioMapData, OutResultMessages))
		{
			for(const UFaceFXAnim* Animation : Asset->Animations)
			{
				if(Animation && !IsAnimationExistInAudioMap(Animation, AudioMapData))
				{
					//not listed in audio map file -> animation not part of .facefx asset anymore
					OutResultMessages.AddModifyWarning(LOCTEXT("LoadingCompiledActorAnimRemoved", "Linked animation asset does not contain an audio map entry. It may have been moved or deleted. Consider to delete that animation asset."), Animation);
				}
			}
		}
		else
		{
			//audio map file does not exist
			OutResultMessages.AddModifyWarning(LOCTEXT("LoadingCompiledActorAmapMissing", "Audio map file is missing."), Asset);
		}
	}
#endif //FACEFX_USEANIMATIONLINKAGE

    return LoadCompiledActorData(Asset, Folder, OutResultMessages);
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAnim* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	checkf(Asset, TEXT("Missing asset"));

	IFileManager& FileManager = IFileManager::Get();
	if(!FileManager.DirectoryExists(*Folder))
	{
		//file not exist
		OutResultMessages.AddModifyError(FText::Format(LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: {0}"), FText::FromString(Folder)), Asset);

		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not exist: %s"), *Folder);
		return false;
	}

	if(!Asset->IsIdSet())
	{
		//no Ids defined yet -> ask user for group/animation

		//fetch all animation groups
		TArray<FString> AnimGroupIds;
		GetAnimationGroupsInFolder(Folder, nullptr, &AnimGroupIds);

		if(AnimGroupIds.Num() > 0)
		{
			TSharedPtr<FFaceFXComboChoiceWidget> SelectionWidget;
			const EAppReturnType::Type SelectResult = FFaceFXComboChoiceWidget::Create(LOCTEXT("SelectAnimTitle", "Select Animation"), LOCTEXT("SelectAnimComboTitle", "Animations:"), AnimGroupIds, SelectionWidget, true);

			if(SelectResult == EAppReturnType::Ok)
			{
				if(FString* Selection = SelectionWidget->GetSelectedOption())
				{
					const bool ParseResult = Asset->GetId().SetFromIdString(*Selection);
					checkf(ParseResult, TEXT("Internal Error. Parsing <group.animId> failed."));
				}
				else
				{
					//no valid response
					OutResultMessages.AddModifyError(LOCTEXT("NoAnimGroupSelected", "No Animation Groups Selected"), Asset);
					return false;
				}
			}
			else
			{
				//canceled
				OutResultMessages.AddModifyError(LOCTEXT("ImportCancelled", "Import Cancelled"), Asset);
				return false;
			}
		}
		else
		{
			//no groups
			OutResultMessages.AddModifyError(LOCTEXT("NoAnimGroups", "No Animation Groups Found"), Asset);
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not contain any animation group: %s"), *Folder);
			return false;
		}
	}

	if(!Asset->IsIdSet())
	{
		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX asset does not have a group/animation id assigned."));
		return false;
	}

#if FACEFX_DELETE_IMPORTED_ANIM
	//The list of successfully imported files. Those will be deleted afterwards
	TArray<FString> ImportedFiles;
#else
	//reset platform data when we don't need to reuse existing data
	Asset->Reset();
#endif //FACEFX_DELETE_IMPORTED_ANIM

	//indicator if import succeeded
	bool allSuccess = true;

	const FString File = GetAnimAssetFileName(Folder, Asset->GetGroup().ToString(), Asset->GetName().ToString());
	const bool FileExists = FPaths::FileExists(File);

#if FACEFX_DELETE_IMPORTED_ANIM
	if(FileExists)
	{
		ImportedFiles.Add(File);
	}
#endif //FACEFX_DELETE_IMPORTED_ANIM

	//check for existing amap entry
	TArray<FFaceFXAudioMapEntry> AudioMapData;
	if(!LoadAudioMapData(Asset, Folder, AudioMapData, OutResultMessages))
	{
		//audio map file does not exist
		OutResultMessages.AddModifyError(LOCTEXT("LoadingCompiledAssetAmapMissing", "Audio map file is missing."));
		allSuccess = false;
	}

	if(!IsAnimationExistInAudioMap(Asset, AudioMapData))
	{
		//not listed in audio map file -> animation not part of .facefx asset anymore
		OutResultMessages.AddModifyWarning(LOCTEXT("LoadingCompiledAssetAnimRemoved", "Skipped .ffxanim file with no audio map entry. It may have been moved or deleted."));
		allSuccess = false;
	}

	if(!FileExists)
	{
#if FACEFX_DELETE_IMPORTED_ANIM
		//source file does not exist -> we consider this asset as being up-to-date and keep it as long as the current data is valid and its listed inside the audio map file
		const FFaceFXAnimData& Data = Asset->GetData();
		if(!Data.IsValid())
#endif //FACEFX_DELETE_IMPORTED_ANIM
		{
			//Data does not exist or is invalid so we actually miss the import data -> reset whole asset again
			OutResultMessages.AddModifyError(FText::Format(LOCTEXT("LoadingCompiledAssetFailedMissing", "Loading initial animation data failed. Source does not exist: {0}"),
			                                               FText::FromString(File)), Asset);
			allSuccess = false;
		}
	}

	//fetch target data container reset in case it existed already (reimport with new data)
	FFaceFXAnimData& Data = Asset->GetData();
	Data.Reset();

	if(!FFileHelper::LoadFileToArray(Data.RawData, *File))
	{
		//failed
		OutResultMessages.AddModifyError(FText::Format(LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. File: {0}"),
		                                               FText::FromString(File)));

		Asset->Reset();
		return false;
	}

#if FACEFX_DELETE_IMPORTED_ANIM
	for(const FString& ImportedFile : ImportedFiles)
	{
		FileManager.Delete(*ImportedFile, false, true, true);
	}
#endif //FACEFX_DELETE_IMPORTED_ANIM

	if(!allSuccess)
	{
		//reset asset again
		Asset->Reset();
		return false;
	}

	//as a last step load the audio asset if desired (settings) and supported by the selected audio system
	return !UFaceFXEditorConfig::Get().IsImportAudio() || !FFaceFXAudio::IsUsingSoundWaveAssets() || LoadAudio(Asset, Folder, OutResultMessages);
}

bool FFaceFXEditorTools::LoadAudio(UFaceFXAnim* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	TArray<FFaceFXAudioMapEntry> AudioMapData;
	if(LoadAudioMapData(Asset, Folder, AudioMapData, OutResultMessages))
	{
		const FString AnimGroup = Asset->GetGroup().ToString();
		const FString AnimId = Asset->GetName().ToString();

		for(const FFaceFXAudioMapEntry& AudioMapEntry : AudioMapData)
		{
			if(!AudioMapEntry.Group.Equals(AnimGroup) || !AudioMapEntry.AnimationId.Equals(AnimId))
			{
				//wrong entry
				continue;
			}

			if (Asset->AudioPath.Equals(AudioMapEntry.AudioPath) && Asset->IsAudioAssetSet())
			{
				//check if the existing asset ref actually exist (may have been deleted)
				const bool IsAudioAssetValid = Asset->GetAudio().LoadSynchronous() != nullptr;
				if (IsAudioAssetValid)
				{
					//no change
					OutResultMessages.AddModifySuccess(LOCTEXT("LoadAudioFailedImportNoChange", "Animation already links to the right Audio Asset"), Asset, Asset->Audio);
					return true;
				}
			}

			//set new path and reset audio link
			Asset->AudioPath = AudioMapEntry.AudioPath;
			Asset->Audio.Reset();

			if(!AudioMapEntry.AudioPath.IsEmpty())
			{
				//lookup audio
				FString AudioFile;
				if(Asset->GetAbsoluteAudioPath(AudioFile) && FPaths::FileExists(AudioFile))
				{
					//try to find sound asset
					if(UFaceFXEditorConfig::Get().IsImportLookupAudio())
					{
						//try to lookup audio
						Asset->Audio = LocateAudio(AudioFile);
					}

					if(!Asset->IsAudioAssetSet())
					{
						//unassigned -> create new asset
						IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

						FString NewAssetName, NewPackageName;
						AssetTools.CreateUniqueAssetName(Asset->GetOutermost()->GetName(), TEXT("_Audio"), NewPackageName, NewAssetName);

						TArray<FString> ImportAssetPath;
						ImportAssetPath.Add(AudioFile);

						//Rather hacky workaround for the fact that importer process is creating the target package name itself and potentially clash with existing files
						//So we import the assets into temp sub folders, rename them to the actual target package and clear the temp folder again
						TArray<UObject*> ImportedAssets = AssetTools.ImportAssets(ImportAssetPath, NewPackageName, nullptr, false);
						USoundWave* ImportedAsset = ImportedAssets.Num() > 0 ? Cast<USoundWave>(ImportedAssets[0]) : nullptr;

						if(ImportedAsset)
						{
							const FString FolderPath = FPaths::GetPath(FPackageName::LongPackageNameToFilename(ImportedAsset->GetOutermost()->GetName()));

							SavePackage(ImportedAsset->GetOutermost(), false);

							//Rename to the actual target package name
							TArray<FAssetRenameData> RenameAssets;
							RenameAssets.Add(FAssetRenameData(ImportedAsset, FPaths::GetPath(NewPackageName), NewAssetName));
							AssetTools.RenameAssets(RenameAssets);

							IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
							AssetRegistry.RemovePath(*NewPackageName);

							//Remove import folder from asset registry again when its empty
							if(IsFolderEmpty(FolderPath))
							{
								IFileManager::Get().DeleteDirectory(*FolderPath);
							}

							if (FSlateApplication::IsInitialized())
							{
								//sync back to the original asset
								IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
								ContentBrowser.SyncBrowserToAssets(ImportedAssets);
							}

							//finally assign new audio
							Asset->Audio = ImportedAsset;

							SavePackage(ImportedAsset->GetOutermost());

							OutResultMessages.AddCreateSuccess(FText::Format(LOCTEXT("LoadAudioFailedImportSuccess", "Importing audio file succeeded. File: {0}"), FText::FromString(AudioFile)), Asset, Asset->Audio);
						}
						else
						{
							UE_LOG(LogFaceFX, Warning, TEXT("FFaceFXEditorTools::LoadAudio. Importing audio file failed. File: %s. Asset: %s"), *AudioFile, *GetNameSafe(Asset));
							OutResultMessages.AddCreateWarning(FText::Format(LOCTEXT("LoadAudioFailedImportFail", "Importing audio asset failed. File: {0}"), FText::FromString(AudioFile)), Asset);
						}
					}
					else
					{
						OutResultMessages.AddModifySuccess(FText::Format(LOCTEXT("LookupAudioSucceded", "Existing USoundWave asset found for target audio data. File: {0}"), FText::FromString(AudioFile)), Asset, Asset->Audio);
					}
				}
				else
				{
					UE_LOG(LogFaceFX, Warning, TEXT("FFaceFXEditorTools::LoadAudio. Audio file does not exist. File: %s. Asset: %s"), *AudioFile, *GetNameSafe(Asset));
					OutResultMessages.AddCreateWarning(FText::Format(LOCTEXT("LoadAudioFailedAudioMissing", "Audio file does not exist. File: {0}"), FText::FromString(AudioFile)), Asset);
				}
			}

			//mapping entry found and either the audio file was found or not. If not ignore this audio entry completely
			return true;
		}
	}

	return false;
}

TSoftObjectPtr<USoundWave> FFaceFXEditorTools::LocateAudio(const FString& AudioSourceFile)
{
	TArray<FAssetData> SoundAssets;

	IFileManager& FileManager = IFileManager::Get();
	const FString AudioSourceFileAbs = FileManager.ConvertToAbsolutePathForExternalAppForRead(*AudioSourceFile);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	if(AssetRegistry.GetAssetsByClass(USoundWave::StaticClass()->GetFName(), SoundAssets))
	{
		for(const FAssetData& AssetData : SoundAssets)
		{
			USoundWave* Asset = Cast<USoundWave>(AssetData.GetAsset());

			if(!Asset)
			{
				//synchronously load sound asset
				Asset = Cast<USoundWave>(StaticLoadObject(USoundWave::StaticClass(), nullptr, *AssetData.ToSoftObjectPath().ToString()));
			}

			checkf(Asset, TEXT("Internal Error. USoundWave asset not loaded. %s"), *AssetData.ToSoftObjectPath().ToString());

			const FString AssetFileAbs = *Asset->AssetImportData->GetFirstFilename();
			if(AssetFileAbs.Equals(AudioSourceFileAbs, ESearchCase::IgnoreCase))
			{
				return Asset;
			}
		}
	}

	UE_LOG(LogFaceFX, Warning, TEXT("FFaceFXEditorTools::LocateAudio. Unable to find USoundWave assets for: %s. Creating a new one..."), *AudioSourceFile);

	return nullptr;
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAnim* Asset, const FName& Group, const FName& Animation, const FString& Folder, FFaceFXImportResult& OutResultMessages)
{
	checkf(Asset, TEXT("Missing asset"));

	if(Group.IsNone() || Animation.IsNone())
	{
		OutResultMessages.AddModifyError(LOCTEXT("MissingGroupAnimation", "Group or Animation ID missing."), Asset);
		return false;
	}
	Asset->Id.Group = Group;
	Asset->Id.Name = Animation;
	return LoadFromCompilationFolder(Asset, Folder, OutResultMessages);
}

bool FFaceFXEditorTools::InitializeFromFile(UFaceFXAsset* Asset, const FString& File, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback, bool IsAssetNew)
{
	checkf(Asset, TEXT("Missing asset"));

	if(!FPaths::FileExists(File))
	{
		//file not exist
		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::InitializeFromFile. FaceFX asset file does not exist: %s"), *File);
		return false;
	}

	Asset->AssetFolder = FPaths::GetPath(File);
	Asset->AssetName = FPaths::GetBaseFilename(File);

	const bool Result = ImportFaceFXAsset(Asset, File, OutResultMessages, BeforeDeletionCallback) && Asset->IsValid();

	if(Result)
	{
		const FText Msg = LOCTEXT("InitFaceFXAssetSuccess", "Initializing FaceFX asset succeeded.");
		if(IsAssetNew)
		{
			OutResultMessages.AddCreateSuccess(Msg, Asset);
		}
		else
		{
			OutResultMessages.AddModifySuccess(Msg, Asset);
		}
	}
	else
	{
		const FText Msg = LOCTEXT("InitFaceFXAssetFailure", "Initializing FaceFX asset failed.");
		if(IsAssetNew)
		{
			OutResultMessages.AddCreateError(Msg, Asset);
		}
		else
		{
			OutResultMessages.AddModifyError(Msg, Asset);
		}
	}

	return Result;
}

void FFaceFXEditorTools::ContentBrowserFocusAsset(const FSoftObjectPath& Asset)
{
	//UObject* AssetObj = Asset.ResolveObject();
	UObject* AssetObj = nullptr;
	FAssetData AssetData(AssetObj);

	if(!AssetObj)
	{
		const FString AssetStr = Asset.ToString();
		AssetData.ObjectPath = FName(*AssetStr);
		AssetData.PackagePath = FName(*FPackageName::GetLongPackagePath(AssetStr));
		AssetData.PackageName = FName(*FPackageName::GetShortName(AssetStr));
	}

	TArray<FAssetData> AssetDataList;
	AssetDataList.Add(AssetData);

	GEditor->SyncBrowserToObjects(AssetDataList);
}

void FFaceFXEditorTools::DeleteAsset(UObject* Asset)
{
	check(Asset);

	TArray<UObject*> Objects;
	Objects.Add(Asset);
	ObjectTools::ForceDeleteObjects(Objects, false);
}

void FFaceFXEditorTools::ShowError(const FText& Msg)
{
	FNotificationInfo Info(Msg);
	Info.ExpireDuration = 10.F;
	Info.bUseLargeFont = false;
	Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

void FFaceFXEditorTools::ShowInfo(const FText& Msg)
{
	FNotificationInfo Info(Msg);
	Info.ExpireDuration = 10.F;
	Info.bUseLargeFont = false;
	Info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE
