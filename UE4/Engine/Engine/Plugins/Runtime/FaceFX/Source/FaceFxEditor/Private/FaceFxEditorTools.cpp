#include "FaceFxEditor.h"

#include "FaceFx.h"
#include "Include/Slate/FaceFxDialogs.h"
#include "FaceFxEditorTools.h"
#include "FaceFxCharacter.h"
#include "FaceFxActor.h"
#include "SlateBasics.h"
#include "ObjectTools.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"

#pragma optimize("", off)

#define LOCTEXT_NAMESPACE "FaceFx"

#define FACEFX_CONFIG_NS TEXT("ThirdParty.FaceFX")

/**
* Gets the path to the FaceFX Studio installation. Configurable via engine ini file (section "ThirdParty.FaceFX", property "StudioPathAbsolute")
* @returns The path to the installation
*/
const FString& FFaceFxEditorTools::GetFaceFXStudioPath()
{
	static FString faceFxPath = TEXT("C:/Program Files (x86)/FaceFX/FaceFX 2015/facefx-studio.exe");
	static bool isLoaded = false;
	if(!isLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetString(FACEFX_CONFIG_NS, TEXT("StudioPathAbsolute"), faceFxPath, GEngineIni);
		isLoaded = true;
	}

	return faceFxPath;
}

/**
* Gets the path to the FaceFX compiler. Configurable via engine ini file (section "ThirdParty.FaceFX", property "CompilerPathRelative")
* @returns The path to the FaceFX compiler
*/
const FString& FFaceFxEditorTools::GetFaceFXCompilerPath()
{
	//default compiler path
	static FString faceFxPath = TEXT("Binaries/ThirdParty/FaceFX/tools/compiler/bin/windows/vs11/x64/Release/ffxc.exe");
	
	static bool isLoaded = false;
	if(!isLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetString(FACEFX_CONFIG_NS, TEXT("CompilerPathRelative"), faceFxPath, GEngineIni);

		faceFxPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / faceFxPath);
		isLoaded = true;
	}

	return faceFxPath;
}

/**
* Gets the timeout duration set in seconds. Default is 120sec. Configurable via engine ini file (section "ThirdParty.FaceFX", property "CompilationTimeoutSec")
* @returns The current duration
*/
float FFaceFxEditorTools::GetCompilationTimeoutDuration()
{
	static float duration = 120.F;

	static bool isLoaded = false;
	if(!isLoaded)
	{
		//try once to fetch overrides from engine ini
		GConfig->GetFloat(FACEFX_CONFIG_NS, TEXT("CompilationTimeoutSec"), duration, GEngineIni);
		isLoaded = true;
	}
	return duration;
}

/**
* Opens the given FaceFX asset within FaceFX studio in case this is installed locally
* @param asset The asset to open
* @param outErrorMessage The optional error message in case it failed
* @returns True if open succeeded, else false
*/
bool FFaceFxEditorTools::OpenFaceFXStudio(UFaceFxActor* asset, FString* outErrorMessage)
{
	if(!asset)
	{
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	const FString studioPath = GetFaceFXStudioPath();
	if(!FPaths::FileExists(studioPath))
	{
		//asset does not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "StartFailStudioMissing", "FaceFX start failed: FaceFX Studio was not found: '%s'.").ToString(), *studioPath);
		}
		return false;
	}

	if(!asset->IsAssetPathSet())
	{
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "StartFailNotSet", "FaceFX start failed: Asset source is not set. Check asset and assign a new source.").ToString();
		}
		return false;
	}

	const FString assetPath = FPaths::ConvertRelativePathToFull(asset->GetAssetPath());
	if(!FPaths::FileExists(assetPath))
	{
		//asset does not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "StartFailMissing", "FaceFX start failed: Asset source file does not exist: '%s'.").ToString(), *assetPath);
		}
		return false;
	}

	const FString args = TEXT("\"") + assetPath + TEXT("\"");

	//create startup script file
	const FProcHandle process = FPlatformProcess::CreateProc(*studioPath, *args, true, true, false, nullptr, 0, nullptr, nullptr);
	if(!process.IsValid())
	{
		//process creation failed
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "StartFail", "FaceFX start failed: Creating the process failed.").ToString();
		}
		return false;
	}

	return true;
}

/**
* Reimports a given FaceFX asset
* @param target The target asset to reimport
* @param outErrorMessage The optional error message in case it failed
* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
* @returns True if open succeeded, else false
*/
bool FFaceFxEditorTools::ReImportFaceFXAsset(UFaceFxAsset* target, FString* outErrorMessage, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback)
{
	if(!target)
	{
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	return ImportFaceFXAsset(target, target->GetAssetPath(), outErrorMessage, beforeDeletionCallback);
}

/**
* Imports a given FaceFX asset
* @param target The target asset to import into
* @param assetPath The path to the .facefx asset file
* @param outErrorMessage The optional error message in case it failed
* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
* @returns True if open succeeded, else false
*/
bool FFaceFxEditorTools::ImportFaceFXAsset(UFaceFxAsset* target, const FString& assetPath, FString* outErrorMessage, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback)
{
	if(!target)
	{
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "MissingAsset", "Target asset missing.").ToString();
		}
		return false;
	}

	FString compiledFile;

	bool result = false;
	if(CompileAssetToTempFolder(assetPath, compiledFile, outErrorMessage))
	{
		//compilation succeeded. Initialize from folder
		result = LoadFromCompilationFolder(target, compiledFile, outErrorMessage);
	}
	
	beforeDeletionCallback.ExecuteIfBound(target, compiledFile, result);

	//remove temp folder again
	IFileManager::Get().DeleteDirectory(*FPaths::ConvertRelativePathToFull(FPaths::GetPath(compiledFile)), true, true);

	//check if the temp folder is empty, if so delete
	struct FDirEmptyCheck : public IPlatformFile::FDirectoryVisitor
	{
		bool m_isEmpty;
		FDirEmptyCheck() : m_isEmpty(true) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			m_isEmpty = false;
			return false;
		}
	} dirIterator;

	const FString tmpFolder = FPaths::ConvertRelativePathToFull(GetTempFolder());
	TArray<FString> files;
	IFileManager::Get().IterateDirectory(*tmpFolder, dirIterator);
	if(dirIterator.m_isEmpty)
	{
		IFileManager::Get().DeleteDirectory(*tmpFolder, true, true);
	}
	return result;
}

/**
* Compiles the given .facefx asset file within a new temporary folder
* @param assetPath The file path
* @param outCompiledFile The temporary file which will be in the temporary folder that also contain all the compiled assets
* @param outErrorMessage The optional error message in case it failed
* @returns True if open succeeded, else false
*/
bool FFaceFxEditorTools::CompileAssetToTempFolder(const FString& assetPath, FString& outCompiledFile, FString* outErrorMessage)
{
	if(!IsFaceFXCompilerInstalled())
	{
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "FaceFxCompilerMissing", "FaceFX asset compiler executable does not exist. Path: %s").ToString(), *GetFaceFXCompilerPath());
		}
		return false;
	}

	if(!FPaths::FileExists(assetPath))
	{
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "MissingAssetNamed", "FaceFX asset file does not exist: %s").ToString(), *assetPath);
		}
		return false;
	}

	//generate temp file names
	const FString tmpDir = FPaths::CreateTempFilename(*GetTempFolder(), TEXT("Compilation"));
	const FString targetAsset = tmpDir / FPaths::GetCleanFilename(assetPath);

	outCompiledFile = tmpDir / FPaths::GetBaseFilename(assetPath);

	//copy face fx asset to the new folder
	if(IFileManager::Get().Copy(*targetAsset, *assetPath, true, true) != COPY_OK)
	{
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "CopyAssetFailed", "Unable to copy asset file.\nSource: %s\nTarget: %s").ToString(), *assetPath, *targetAsset);
		}
		return false;
	}

	//ffxc.exe -p=x86,ps4 --idmap actor.facefx
	const FString args = FString::Printf(TEXT("-p=x86,ps4,xboxone --idmap \"%s\""), *FPaths::ConvertRelativePathToFull(targetAsset));

	FProcHandle runningProc = FPlatformProcess::CreateProc(*GetFaceFXCompilerPath(), *args, true, false, false, nullptr, 0, *tmpDir, nullptr);
	if(!runningProc.IsValid())
	{
		//process creation failed
		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "StartFail", "Creating the FaceFX asset compiler process failed.").ToString();
		}
		return false;
	}

	//TODO: Make the load/compilation phase non blocking
	//wait till process finished for some time
	const double timeOut = GetCompilationTimeoutDuration();
	const double startSeconds = FPlatformTime::Seconds();
	while(FPlatformProcess::IsProcRunning(runningProc) && (FPlatformTime::Seconds() - startSeconds < timeOut))
	{
		//wait
		FPlatformProcess::Sleep(0.1F);
		// Tick slate
		FPlatformMisc::PumpMessages(true);
		FSlateApplication::Get().Tick();
		FSlateApplication::Get().GetRenderer()->Sync();
	};

	if(FPlatformProcess::IsProcRunning(runningProc))
	{
		//process still running -> kill it
		FPlatformProcess::TerminateProc(runningProc);

		if(outErrorMessage)
		{
			*outErrorMessage = NSLOCTEXT("FaceFx", "FaceFxCompilerTimeout", "The FaceFX asset compiler process did not finished within the maximum time frame.\nConsider to extend the timeout duration setting within DefaultEngine.ini, property CompilationTimeoutSec").ToString();
		}
		return false;
	}

	//check for compiler return codes
	int32 retCode = INDEX_NONE;
	if(!FPlatformProcess::GetProcReturnCode(runningProc, &retCode) || retCode != 0)
	{
		//execution returned non zero (error)
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "FaceFxCompilerReturn", "The FaceFX asset compiler process returned with error code %i.").ToString(), retCode);
		}
		return false;
	}

	return true;
}


/** 
* Loads all assets from a given folder into the given target struct
* @param folder The path to the folder to load the compiled assets from
* @param assetName The name of the assets to load without any extension
* @param targetData The target struct to load into
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadCompiledData(const FString& folder, const FString& assetName, FFaceFxActorData& targetData)
{
	//reset all
	targetData.m_ids.Empty();
	targetData.m_bonesRawData.Empty();
	targetData.m_actorRawData.Empty();

	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//folder not exist
		return false;
	}

	//load .ffxactor
	const FString assetPathActor = FPaths::Combine(*folder, *(assetName + ".ffxactor"));
	if(FPaths::FileExists(assetPathActor))
	{
		FFileHelper::LoadFileToArray(targetData.m_actorRawData, *assetPathActor);
	}

	//load .ffxbones
	const FString assetPathBones = FPaths::Combine(*folder, *(assetName + ".ffxbones"));
	if(FPaths::FileExists(assetPathBones))
	{
		FFileHelper::LoadFileToArray(targetData.m_bonesRawData, *assetPathBones);
	}

	//load .ffxids
	const FString assetPathIds = FPaths::Combine(*folder, *(assetName + ".ffxids"));
	if(FPaths::FileExists(assetPathIds))
	{
		TArray<FString> lines;
		if(FFileHelper::LoadANSITextFileToStrings(*assetPathIds, nullptr, lines))
		{
			for(const FString line : lines)
			{
				FString ids, name;
				if(line.Split(":", &ids, &name))
				{
					if(ids.Len() <= 16)
					{
						//accept only max 8 hex values as input
						uint64 id = 0;
						while(ids.Len() > 0)
						{
							id = id << 8;
							id |= FParse::HexNumber(*(ids.Left(2)));
							ids = ids.RightChop(2);
						}
						targetData.m_ids.Add(FFaceFxIdData(id, FName(*name)));
					}
				}
			}
		}
	}
	return true;
}

/** 
* Loads all assets from a given folder into the given target struct
* @param folder The path to the folder to load the compiled assets from
* @param targetData The target struct to load into
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadCompiledData(const FString& folder, FFaceFxAnimSetData& targetData)
{
	//reset all
	targetData.m_animations.Empty();

	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//folder not exist
		return false;
	}

	//load all animations within the folder
	TArray<FString> filenames;
	IFileManager::Get().FindFiles(filenames, *(folder / TEXT("*.ffxanim")), true, false);

	for(const FString& file : filenames)
	{
		FFaceFxAnimData entry;
		entry.m_name = FName(*FPaths::GetBaseFilename(file));
		if(FFileHelper::LoadFileToArray(entry.m_rawData, *FPaths::Combine(*folder, *file)))
		{
			targetData.m_animations.Add(entry);
		}
		else
		{
			UE_LOG(LogFaceFx, Error, TEXT("YFaceFx::LoadFaceFxCompiledData. Loading FaceFX animation file failed. The file will be ignored and not being part of the asset. File: %s"), *file);
		}
	}

	return true;
}

/**
* Gets all animation groups found within a given FaceFx compilation folder
* @param folder The compilation folder to load from
* @param outGroups The resulting animation groups
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::GetAnimationGroupsInFolder(const FString& folder, TArray<FString>& outGroups)
{
	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//folder not exist
		return false;
	}

	//load all sub folders
	TArray<FString> directories;
	IFileManager::Get().FindFiles(directories, *(folder / TEXT("*")), false, true);

	//load all animations within the sub folders
	for(const FString& dir : directories)
	{
		TArray<FString> filenames;
		IFileManager::Get().FindFiles(filenames, *(folder / dir / TEXT("*.ffxanim")), true, false);

		if(filenames.Num() > 0)
		{
			outGroups.Add(FPaths::GetCleanFilename(dir));
		}
	}

	return true;
}

/**
* Creates a new anim set asset
* @param compilationFolder The folder to load the data from
* @param animGroup The animation group folder to load the data from
* @param packageName The name of the package
* @param fxActor The FaceFX actor asset that shall be linked to that new asset
* @param assetTools The asset tools instance to use
* @param factory The factory to use. Keep at nullptr to directly create an instance
*/
UFaceFxAnimSet* FFaceFxEditorTools::CreateAnimSetAsset(const FString& compilationFolder, const FString& animGroup, const FString& packageName, UFaceFxActor* fxActor, IAssetTools& assetTools, UFactory* factory)
{
	if(!fxActor)
	{
		return nullptr;
	}

	//remove any blanks
	const FString groupNoBlanks = animGroup.Replace(TEXT(" "), TEXT(""), ESearchCase::CaseSensitive);

	//create new asset for this group
	FString newAssetName, newPackageName;
	assetTools.CreateUniqueAssetName(packageName, TEXT("_Anims_") + groupNoBlanks, newPackageName, newAssetName);

	//FAssetTools::CreateAsset is concatenating the /<assetname> of the packagepath itself, hence we remove it
	newPackageName.RemoveFromEnd(TEXT("/") + newAssetName);

	if(UFaceFxAnimSet* newAsset = Cast<UFaceFxAnimSet>(assetTools.CreateAsset(newAssetName, newPackageName, UFaceFxAnimSet::StaticClass(), factory)))
	{
		if(LoadFromCompilationFolder(newAsset, FName(*animGroup), compilationFolder))
		{
			//copy asset sources (for reimport etc)
			newAsset->SetSources(fxActor->GetAssetName(), fxActor->GetAssetFolder());

			SavePackage(newAsset->GetOutermost());

			//link to the new asset
			fxActor->LinkTo(newAsset);

			return newAsset;
		}
		else
		{
			//delete asset again
			ObjectTools::DeleteSingleObject(newAsset, false);
		}
	}

	return nullptr;
}

/**
* Performs steps to store an asset physically and in source control
* @param package The package to save
*/
void FFaceFxEditorTools::SavePackage(UPackage* package)
{
	checkf(package, TEXT("Missing package"));

	package->MarkPackageDirty();
	package->PostEditChange();

	const FString packagePath = package->GetPathName();
	const FString filename = FPackageName::LongPackageNameToFilename(packagePath, FPackageName::GetAssetPackageExtension());
	GEditor->SavePackage(package, nullptr, RF_Standalone, *filename, GWarn);

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if (ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable() )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(filename, EStateCacheUsage::ForceUpdate);
		if (!SourceControlState->IsCheckedOut() && !SourceControlState->IsAdded())
		{
			TArray<FString> files;
			files.Add(FPaths::ConvertRelativePathToFull(filename));
			SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), files);
		}
	}
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
* @param asset The target asset
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadFromCompilationFolder(UFaceFxAsset* asset, const FString& folder, FString* outErrorMessage)
{
	checkf(asset, TEXT("Missing asset"));

	if(auto fxActor = Cast<UFaceFxActor>(asset))
	{
		return LoadFromCompilationFolder(fxActor, folder, outErrorMessage);
	}
	
	if(auto animSet = Cast<UFaceFxAnimSet>(asset))
	{
		return LoadFromCompilationFolder(animSet, folder, outErrorMessage);
	}
	return false;
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
* @param asset The target asset
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadFromCompilationFolder(UFaceFxActor* asset, const FString& folder, FString* outErrorMessage)
{
	checkf(asset, TEXT("Missing asset"));

	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//file not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *folder);
		}
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxActor::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	//reset platform data
	asset->m_platformData.Reset(EFaceFxTargetPlatform::MAX);
	asset->m_platformData.AddZeroed(EFaceFxTargetPlatform::MAX);

	bool result = true;

	for(int8 i=0; i<EFaceFxTargetPlatform::MAX; ++i)
	{
		const EFaceFxTargetPlatform::Type target = EFaceFxTargetPlatform::Type(i);
		if(!LoadCompiledData(GetCompilationFolder(folder, target), asset->m_assetName, asset->m_platformData[target]))
		{
			result = false;

			if(outErrorMessage)
			{
				*outErrorMessage = FString::Printf(*LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. Folder: %s").ToString(), *folder);
			}
		}
	}
	return result;
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
* @param asset The target asset
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadFromCompilationFolder(UFaceFxAnimSet* asset, const FString& folder, FString* outErrorMessage)
{
	checkf(asset, TEXT("Missing asset"));

	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//file not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *folder);
		}
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	if(asset->m_group.IsNone())
	{
		//no group defined yet -> ask user for group

		//fetch all animation groups
		TArray<FString> groups;
		GetAnimationGroupsInFolder(folder, EFaceFxTargetPlatform::PC, groups);

		if(groups.Num() > 0)
		{
			auto selectionWidget = SNew(FComboboxChoiceWidget)
				.Options(groups)
				.Message(LOCTEXT("SelectAnimGroupComboTitle", "Animation Groups:"));

			const EAppReturnType::Type result = selectionWidget->OpenModalDialog(LOCTEXT("SelectAnimGroupTitle", "Select Animation Group"));

			if(result == EAppReturnType::Ok)
			{
				if(FString* selection = selectionWidget->GetSelectedOption())
				{
					asset->m_group = FName(**selection);
				}
				else
				{
					//no valid response
					if(outErrorMessage)
					{
						*outErrorMessage = LOCTEXT("NoAnimGroupSelected", "No Animation Groups Selected").ToString();
					}
					return false;
				}
			}
			else
			{
				//canceled
				if(outErrorMessage)
				{
					*outErrorMessage = LOCTEXT("ImportCancelled", "Import Cancelled").ToString();
				}
				return false;
			}
		}
		else
		{
			//no groups
			if(outErrorMessage)
			{
				*outErrorMessage = LOCTEXT("NoAnimGroups", "No Animation Groups Found").ToString();
			}
			UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not contain any animation group: %s"), *folder);
			return false;
		}
	}

	if(asset->m_group.IsNone())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	//reset platform data
	asset->m_platformData.Reset(EFaceFxTargetPlatform::MAX);
	asset->m_platformData.AddZeroed(EFaceFxTargetPlatform::MAX);

	bool result = true;

	for(int8 i=0; i<EFaceFxTargetPlatform::MAX; ++i)
	{
		const EFaceFxTargetPlatform::Type target = static_cast<EFaceFxTargetPlatform::Type>(i);
		if(!LoadCompiledData(GetCompilationFolder(folder, target) / asset->m_group.GetPlainNameString(), asset->m_platformData[target]))
		{
			result = false;

			if(outErrorMessage)
			{
				*outErrorMessage = FString::Printf(*LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. Folder: %s").ToString(), *folder);
			}
		}
	}
	return result;
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder. Assigns a new group before doing so
* @param asset The target asset
* @param group The new group
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::LoadFromCompilationFolder(UFaceFxAnimSet* asset, const FName& group, const FString& folder, FString* outErrorMessage)
{
	checkf(asset, TEXT("Missing asset"));

	if(group.IsNone())
	{
		return false;
	}
	asset->m_group = group;
	return LoadFromCompilationFolder(asset, folder, outErrorMessage);
}

/** 
* Initializes the asset from a .facefx asset file
* @param asset The target asset
* @param file The path to the .facefx asset file
* @param errorMessage The optional error message in case it failed
* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
* @returns True if succeeded, else false
*/
bool FFaceFxEditorTools::InitializeFromFile(UFaceFxAsset* asset, const FString& file, FString* outErrorMessage, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback)
{
	checkf(asset, TEXT("Missing asset"));

	if(!FPaths::FileExists(file))
	{
		//file not exist
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAsset::InitializeFromFaceFx. FaceFX asset file does not exist: %s"), *file);
		return false;
	}

	asset->m_assetFolder = FPaths::GetPath(file);
	asset->m_assetName = FPaths::GetBaseFilename(file);

	return ImportFaceFXAsset(asset, file, outErrorMessage, beforeDeletionCallback) && asset->IsValid();
}

#undef LOCTEXT_NAMESPACE