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

#include "FaceFXEditor.h"
#include "FaceFXEditorTools.h"

#include "FaceFX.h"
#include "Include/Slate/FaceFXDialogs.h"
#include "FaceFXCharacter.h"
#include "FaceFXActor.h"
#include "SlateBasics.h"
#include "ObjectTools.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "FaceFX"

#define FACEFX_CONFIG_NS TEXT("ThirdParty.FaceFX")

const FString& FFaceFXEditorTools::GetFaceFXStudioPath()
{
	static FString StudioPath = TEXT("C:/Program Files (x86)/FaceFX/FaceFX 2015/facefx-studio.exe");
	static bool bIsLoaded = false;
	if(!bIsLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetString(FACEFX_CONFIG_NS, TEXT("StudioPathAbsolute"), StudioPath, GEngineIni);
		bIsLoaded = true;
	}

	return StudioPath;
}

const FString& FFaceFXEditorTools::GetFaceFXCompilerPath()
{
	//default compiler path
	static FString CompilerPath = FString(TEXT("Runtime/FaceFX/Source/FaceFXLib/")) + TEXT(FACEFX_RUNTIMEFOLDER) + TEXT("/tools/compiler/bin/windows/vs11/x64/Release/ffxc.exe");
	static bool bIsLoaded = false;
	if(!bIsLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetString(FACEFX_CONFIG_NS, TEXT("CompilerPathRelative"), CompilerPath, GEngineIni);

		CompilerPath = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir() / CompilerPath);
		bIsLoaded = true;
	}

	return CompilerPath;
}

float FFaceFXEditorTools::GetCompilationTimeoutDuration()
{
	static float Duration = 120.F;
	static bool bIsLoaded = false;
	if(!bIsLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetFloat(FACEFX_CONFIG_NS, TEXT("CompilationTimeoutSec"), Duration, GEngineIni);
		bIsLoaded = true;
	}
	return Duration;
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

	const FString StudioPath = GetFaceFXStudioPath();
	if(!FPaths::FileExists(StudioPath))
	{
		//asset does not exist
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("StartFailStudioMissing", "FaceFX start failed: FaceFX Studio was not found: '%s'.").ToString(), *StudioPath);
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
			*OutErrorMessage = FString::Printf(*LOCTEXT("StartFailMissing", "FaceFX start failed: Asset source file does not exist: '%s'.").ToString(), *AssetPath);
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

bool FFaceFXEditorTools::ReImportFaceFXAsset(UFaceFXAsset* Asset, FString* OutErrorMessage, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback)
{
	if(!Asset)
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	return ImportFaceFXAsset(Asset, Asset->GetAssetPath(), OutErrorMessage, BeforeDeletionCallback);
}

bool FFaceFXEditorTools::ImportFaceFXAsset(UFaceFXAsset* Asset, const FString& AssetPath, FString* OutErrorMessage, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback)
{
	if(!Asset)
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	FString CompiledFile;

	bool Result = false;
	if(CompileAssetToTempFolder(AssetPath, CompiledFile, OutErrorMessage))
	{
		//compilation succeeded. Initialize from folder
		Result = LoadFromCompilationFolder(Asset, CompiledFile, OutErrorMessage);
	}
	
	BeforeDeletionCallback.ExecuteIfBound(Asset, CompiledFile, Result);

	//remove temp folder again
	IFileManager::Get().DeleteDirectory(*FPaths::ConvertRelativePathToFull(FPaths::GetPath(CompiledFile)), true, true);

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

	const FString TmpFolder = FPaths::ConvertRelativePathToFull(GetTempFolder());
	TArray<FString> Files;
	IFileManager::Get().IterateDirectory(*TmpFolder, DirIterator);
	if(DirIterator.bIsEmpty)
	{
		IFileManager::Get().DeleteDirectory(*TmpFolder, true, true);
	}
	return Result;
}

bool FFaceFXEditorTools::CompileAssetToTempFolder(const FString& AssetPath, FString& OutCompiledFile, FString* OutErrorMessage)
{
	if(!IsFaceFXCompilerInstalled())
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("FaceFXCompilerMissing", "FaceFX asset compiler executable does not exist. Path: %s").ToString(), *GetFaceFXCompilerPath());
		}
		return false;
	}

	if(!FPaths::FileExists(AssetPath))
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("MissingAssetNamed", "FaceFX asset file does not exist: %s").ToString(), *AssetPath);
		}
		return false;
	}

	//generate temp file names
	const FString TmpDir = FPaths::CreateTempFilename(*GetTempFolder(), TEXT("Compilation"));
	const FString TargetAsset = TmpDir / FPaths::GetCleanFilename(AssetPath);

	OutCompiledFile = TmpDir / FPaths::GetBaseFilename(AssetPath);

	//copy face fx asset to the new folder
	if(IFileManager::Get().Copy(*TargetAsset, *AssetPath, true, true) != COPY_OK)
	{
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("CopyAssetFailed", "Unable to copy asset file.\nSource: %s\nTarget: %s").ToString(), *AssetPath, *TargetAsset);
		}
		return false;
	}

	//ffxc.exe -p=x86,ps4 --idmap actor.facefx
	const FString Args = FString::Printf(TEXT("-p=x86,ps4,xboxone --idmap \"%s\""), *FPaths::ConvertRelativePathToFull(TargetAsset));

	FProcHandle RunningProc = FPlatformProcess::CreateProc(*GetFaceFXCompilerPath(), *Args, true, false, false, nullptr, 0, *TmpDir, nullptr);
	if(!RunningProc.IsValid())
	{
		//process creation failed
		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("StartFail", "Creating the FaceFX asset compiler process failed.").ToString();
		}
		return false;
	}

	//TODO: Make the load/compilation phase non blocking
	//wait till process finished for some time
	const double TimeOut = GetCompilationTimeoutDuration();
	const double StartSeconds = FPlatformTime::Seconds();
	while(FPlatformProcess::IsProcRunning(RunningProc) && (FPlatformTime::Seconds() - StartSeconds < TimeOut))
	{
		//wait
		FPlatformProcess::Sleep(0.1F);
		// Tick slate
		FPlatformMisc::PumpMessages(true);
		FSlateApplication::Get().Tick();
		FSlateApplication::Get().GetRenderer()->Sync();
	};

	if(FPlatformProcess::IsProcRunning(RunningProc))
	{
		//process still running -> kill it
		FPlatformProcess::TerminateProc(RunningProc);

		if(OutErrorMessage)
		{
			*OutErrorMessage = LOCTEXT("FaceFXCompilerTimeout", "The FaceFX asset compiler process did not finished within the maximum time frame.\nConsider to extend the timeout duration setting within DefaultEngine.ini, property CompilationTimeoutSec").ToString();
		}
		return false;
	}

	//check for compiler return codes
	int32 RetCode = INDEX_NONE;
	if(!FPlatformProcess::GetProcReturnCode(RunningProc, &RetCode) || RetCode != 0)
	{
		//execution returned non zero (error)
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("FaceFXCompilerReturn", "The FaceFX asset compiler process returned with error code %i.").ToString(), RetCode);
		}
		return false;
	}

	return true;
}

bool FFaceFXEditorTools::LoadCompiledData(const FString& Folder, const FString& AssetName, FFaceFXActorData& TargetData)
{
	//reset all
	TargetData.Ids.Empty();
	TargetData.BonesRawData.Empty();
	TargetData.ActorRawData.Empty();

	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//folder not exist
		return false;
	}

	//load .ffxactor
	const FString AssetPathActor = FPaths::Combine(*Folder, *(AssetName + ".ffxactor"));
	if(FPaths::FileExists(AssetPathActor))
	{
		FFileHelper::LoadFileToArray(TargetData.ActorRawData, *AssetPathActor);
	}

	//load .ffxbones
	const FString AssetPathBones = FPaths::Combine(*Folder, *(AssetName + ".ffxbones"));
	if(FPaths::FileExists(AssetPathBones))
	{
		FFileHelper::LoadFileToArray(TargetData.BonesRawData, *AssetPathBones);
	}

	//load .ffxids
	const FString AssetPathIds = FPaths::Combine(*Folder, *(AssetName + ".ffxids"));
	if(FPaths::FileExists(AssetPathIds))
	{
		TArray<FString> Lines;
		if(FFileHelper::LoadANSITextFileToStrings(*AssetPathIds, nullptr, Lines))
		{
			for(const FString& Line : Lines)
			{
				FString Ids, Name;
				if(Line.Split(":", &Ids, &Name))
				{
					if(Ids.Len() <= 16)
					{
						//accept only max 8 hex values as input
						uint64 Id = 0;
						while(Ids.Len() > 0)
						{
							Id = Id << 8;
							Id |= FParse::HexNumber(*(Ids.Left(2)));
							Ids = Ids.RightChop(2);
						}
						TargetData.Ids.Add(FFaceFXIdData(Id, FName(*Name)));
					}
				}
			}
		}
	}
	return true;
}

bool FFaceFXEditorTools::LoadCompiledData(const FString& Folder, FFaceFXAnimSetData& TargetData)
{
	//reset all
	TargetData.Animations.Empty();

	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//folder not exist
		return false;
	}

	//load all animations within the folder
	TArray<FString> Filenames;
	IFileManager::Get().FindFiles(Filenames, *(Folder / TEXT("*.ffxanim")), true, false);

	for(const FString& File : Filenames)
	{
		FFaceFXAnimData Entry;
		Entry.Name = FName(*FPaths::GetBaseFilename(File));
		if(FFileHelper::LoadFileToArray(Entry.RawData, *FPaths::Combine(*Folder, *File)))
		{
			TargetData.Animations.Add(Entry);
		}
		else
		{
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadCompiledData. Loading FaceFX animation file failed. The file will be ignored and not being part of the asset. File: %s"), *File);
		}
	}

	return true;
}

bool FFaceFXEditorTools::GetAnimationGroupsInFolder(const FString& Folder, TArray<FString>& OutGroups)
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
		IFileManager::Get().FindFiles(Filenames, *(Folder / Dir / TEXT("*.ffxanim")), true, false);

		if(Filenames.Num() > 0)
		{
			OutGroups.Add(FPaths::GetCleanFilename(Dir));
		}
	}

	return true;
}

UFaceFXAnimSet* FFaceFXEditorTools::CreateAnimSetAsset(const FString& CompilationFolder, const FString& AnimGroup, const FString& PackageName, UFaceFXActor* FaceFXActor, IAssetTools& AssetTools, UFactory* Factory)
{
	if(!FaceFXActor)
	{
		return nullptr;
	}

	//remove any blanks
	const FString GroupNoBlanks = AnimGroup.Replace(TEXT(" "), TEXT(""), ESearchCase::CaseSensitive);

	//create new asset for this group
	FString NewAssetName, NewPackageName;
	AssetTools.CreateUniqueAssetName(PackageName, TEXT("_Anims_") + GroupNoBlanks, NewPackageName, NewAssetName);

	//FAssetTools::CreateAsset is concatenating the /<assetname> of the packagepath itself, hence we remove it
	NewPackageName.RemoveFromEnd(TEXT("/") + NewAssetName);

	if(UFaceFXAnimSet* NewAsset = Cast<UFaceFXAnimSet>(AssetTools.CreateAsset(NewAssetName, NewPackageName, UFaceFXAnimSet::StaticClass(), Factory)))
	{
		if(LoadFromCompilationFolder(NewAsset, FName(*AnimGroup), CompilationFolder))
		{
			//copy asset sources (for reimport etc)
			NewAsset->SetSources(FaceFXActor->GetAssetName(), FaceFXActor->GetAssetFolder());

			SavePackage(NewAsset->GetOutermost());

			//link to the new asset
			FaceFXActor->LinkTo(NewAsset);

			return NewAsset;
		}
		else
		{
			//delete asset again
			ObjectTools::DeleteSingleObject(NewAsset, false);
		}
	}

	return nullptr;
}

void FFaceFXEditorTools::SavePackage(UPackage* Package)
{
	checkf(Package, TEXT("Missing package"));

	Package->MarkPackageDirty();
	Package->PostEditChange();

	const FString PackagePath = Package->GetPathName();
	const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	GEditor->SavePackage(Package, nullptr, RF_Standalone, *Filename, GWarn);

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if (ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable() )
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

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAsset* Asset, const FString& Folder, FString* OutErrorMessage)
{
	checkf(Asset, TEXT("Missing asset"));

	if(auto FaceFXActor = Cast<UFaceFXActor>(Asset))
	{
		return LoadFromCompilationFolder(FaceFXActor, Folder, OutErrorMessage);
	}
	
	if(auto AnimSet = Cast<UFaceFXAnimSet>(Asset))
	{
		return LoadFromCompilationFolder(AnimSet, Folder, OutErrorMessage);
	}
	return false;
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXActor* Asset, const FString& Folder, FString* OutErrorMessage)
{
	checkf(Asset, TEXT("Missing asset"));

	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//file not exist
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *Folder);
		}
		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not exist: %s"), *Folder);
		return false;
	}

	//reset platform data
	Asset->PlatformData.Reset(EFaceFXTargetPlatform::MAX);
	Asset->PlatformData.AddZeroed(EFaceFXTargetPlatform::MAX);

	bool Result = true;

	for(int8 i=0; i<EFaceFXTargetPlatform::MAX; ++i)
	{
		const EFaceFXTargetPlatform::Type Target = EFaceFXTargetPlatform::Type(i);
		if(!LoadCompiledData(GetCompilationFolder(Folder, Target), Asset->AssetName, Asset->PlatformData[Target]))
		{
			Result = false;

			if(OutErrorMessage)
			{
				*OutErrorMessage = FString::Printf(*LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. Folder: %s").ToString(), *Folder);
			}
		}
	}
	return Result;
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAnimSet* Asset, const FString& Folder, FString* OutErrorMessage)
{
	checkf(Asset, TEXT("Missing asset"));

	if(!IFileManager::Get().DirectoryExists(*Folder))
	{
		//file not exist
		if(OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *Folder);
		}
		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not exist: %s"), *Folder);
		return false;
	}

	if(Asset->Group.IsNone())
	{
		//no group defined yet -> ask user for group

		//fetch all animation groups
		TArray<FString> Groups;
		GetAnimationGroupsInFolder(Folder, EFaceFXTargetPlatform::PC, Groups);

		if(Groups.Num() > 0)
		{
			auto SelectionWidget = SNew(FComboboxChoiceWidget)
				.Options(Groups)
				.Message(LOCTEXT("SelectAnimGroupComboTitle", "Animation Groups:"));

			const EAppReturnType::Type SelectResult = SelectionWidget->OpenModalDialog(LOCTEXT("SelectAnimGroupTitle", "Select Animation Group"));

			if(SelectResult == EAppReturnType::Ok)
			{
				if(FString* Selection = SelectionWidget->GetSelectedOption())
				{
					Asset->Group = FName(**Selection);
				}
				else
				{
					//no valid response
					if(OutErrorMessage)
					{
						*OutErrorMessage = LOCTEXT("NoAnimGroupSelected", "No Animation Groups Selected").ToString();
					}
					return false;
				}
			}
			else
			{
				//canceled
				if(OutErrorMessage)
				{
					*OutErrorMessage = LOCTEXT("ImportCancelled", "Import Cancelled").ToString();
				}
				return false;
			}
		}
		else
		{
			//no groups
			if(OutErrorMessage)
			{
				*OutErrorMessage = LOCTEXT("NoAnimGroups", "No Animation Groups Found").ToString();
			}
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not contain any animation group: %s"), *Folder);
			return false;
		}
	}

	if(Asset->Group.IsNone())
	{
		UE_LOG(LogFaceFX, Error, TEXT("FFaceFXEditorTools::LoadFromCompilationFolder. FaceFX compilation folder does not exist: %s"), *Folder);
		return false;
	}

	//reset platform data
	Asset->PlatformData.Reset(EFaceFXTargetPlatform::MAX);
	Asset->PlatformData.AddZeroed(EFaceFXTargetPlatform::MAX);

	bool Result = true;

	for(int8 i=0; i<EFaceFXTargetPlatform::MAX; ++i)
	{
		const EFaceFXTargetPlatform::Type Target = static_cast<EFaceFXTargetPlatform::Type>(i);
		if(!LoadCompiledData(GetCompilationFolder(Folder, Target) / Asset->Group.GetPlainNameString(), Asset->PlatformData[Target]))
		{
			Result = false;

			if(OutErrorMessage)
			{
				*OutErrorMessage = FString::Printf(*LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. Folder: %s").ToString(), *Folder);
			}
		}
	}
	return Result;
}

bool FFaceFXEditorTools::LoadFromCompilationFolder(UFaceFXAnimSet* Asset, const FName& Group, const FString& Folder, FString* OutErrorMessage)
{
	checkf(Asset, TEXT("Missing asset"));

	if(Group.IsNone())
	{
		return false;
	}
	Asset->Group = Group;
	return LoadFromCompilationFolder(Asset, Folder, OutErrorMessage);
}

bool FFaceFXEditorTools::InitializeFromFile(UFaceFXAsset* Asset, const FString& File, FString* OutErrorMessage, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback)
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

	return ImportFaceFXAsset(Asset, File, OutErrorMessage, BeforeDeletionCallback) && Asset->IsValid();
}

#undef LOCTEXT_NAMESPACE
