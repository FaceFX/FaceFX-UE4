#include "FaceFx.h"

#if WITH_EDITOR
#include "FaceFx-0.9.10/facefx.h"

#include "FaceFxCharacter.h"
#include "FaceFxActor.h"
#include "SlateBasics.h"
#include "ObjectTools.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"

#define FACEFX_CONFIG_NS TEXT("ThirdParty.FaceFX")

/** A single open FaceFX studio request process. Self deleting once the process finished. */
struct FFaceFxOpenStudioProcess
{
	FFaceFxOpenStudioProcess(const FString& executable, const FString& script, const FString& scriptFile) : m_scriptFile(scriptFile), m_process(executable, script, true)
	{
	}

	/**
	* Launches the process
	* @param deleteOnComplete Indicator if this instance is created on the heap and shall delete itself once the process closed together with the passed script file.
	* When true don't use this reference anymore as its deleting itself once the process finished
	* @returns True if succeeded, else false. 
	*/
	bool Launch(bool deleteOnComplete = true)
	{
		if(m_process.IsRunning())
		{
			return false;
		}
		if(deleteOnComplete)
		{
			m_process.OnCompleted().BindRaw(this, &FFaceFxOpenStudioProcess::OnCompleted);
		}
		const bool result = m_process.Launch();
		if(!result && deleteOnComplete)
		{
			//launch failed so we explicitly call completion
			OnCompleted(INDEX_NONE);
		}
		return result;
	}

private:

	/** The script that was used to open the asset */
	FString m_scriptFile;

	/** The monitored process */
	FMonitoredProcess m_process;

	/** Callback for when the FaceFX studio process finished */
	void OnCompleted(int32 returnCode)
	{
		IFileManager::Get().Delete(*m_scriptFile, false, true, true);
		delete this;
	}
};

/**
* Gets the path to the FaceFX Studio installation. Configurable via engine ini file (section "Yager.ThirdParty.FaceFX", property "StudioPathAbsolute")
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
* Gets the path to the FaceFX compiler. Configurable via engine ini file (section "Yager.ThirdParty.FaceFX", property "CompilerPathRelative")
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
* Gets the timeout duration set in seconds. Default is 120sec. Configurable via engine ini file (section "Yager.ThirdParty.FaceFX", property "CompilationTimeoutSec")
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
* Checks if FaceFX studio is installed within the standard installation path
* @returns True if installed, else false
*/
bool FFaceFxEditorTools::IsFaceFXStudioInstalled()
{
	return FPaths::FileExists(GetFaceFXStudioPath());
}

/**
* Gets the indicator if the compiler is installed
* @returns True if installed else false
*/
bool FFaceFxEditorTools::IsFaceFXCompilerInstalled()
{
	return FPaths::FileExists(GetFaceFXCompilerPath());
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

	const FString studioPath = FFaceFxEditorTools::GetFaceFXStudioPath();

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

	//create startup script file
	const FString pyhtonSc = FString::Printf(TEXT("loadactor -f \"%s\""), *assetPath);
	const FString scriptFile = FPaths::CreateTempFilename(*GetTempFolder(), TEXT("startScript_"), TEXT(".fxl"));

	if(FFileHelper::SaveStringToFile(pyhtonSc, *scriptFile))
	{
		const FString args = FString::Printf(TEXT("-guiexec=\"%s\""), *FPaths::ConvertRelativePathToFull(scriptFile));

		FFaceFxOpenStudioProcess* newProcess = new FFaceFxOpenStudioProcess(studioPath, args, scriptFile);
		if(newProcess->Launch())
		{
			//open asset succeeded
			return true;
		}
		else
		{
			//process creation failed
			if(outErrorMessage)
			{
				*outErrorMessage = NSLOCTEXT("FaceFx", "StartFail", "FaceFX start failed: Creating the process failed.").ToString();
			}
		}
	}
	else
	{
		//creating facefx script failed
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*NSLOCTEXT("FaceFx", "StartFail", "FaceFX start failed: Could not write temporary startup script file '%s'.").ToString(), *scriptFile);
		}
	}

	return false;
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

	FString compiledPath;

	bool result = false;
	if(CompileAssetToTempFolder(assetPath, compiledPath, outErrorMessage))
	{
		//compilation succeeded. Initialize from folder
		result = target->LoadFromFaceFxCompilationFolder(compiledPath, outErrorMessage);
	}
	
	beforeDeletionCallback.ExecuteIfBound(target, compiledPath, result);

	//remove temp folder again
	IFileManager::Get().DeleteDirectory(*FPaths::ConvertRelativePathToFull(compiledPath / ".."), true, true);

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
		if(newAsset->LoadFromFaceFxCompilationFolder(FName(*animGroup), compilationFolder))
		{
			//copy asset sources (for reimport etc)
			newAsset->SetSources(fxActor->GetAssetName(), fxActor->GetAssetFolder());

			FFaceFxEditorTools::SavePackage(newAsset->GetOutermost());

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

#endif //WITH_EDITOR
