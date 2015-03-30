#include "FaceFxUI.h"
#include "FaceFx.h"
#include "AssetTypeActions_FaceFxBase.h"

#include "UnrealEd.h"
#include "IDesktopPlatform.h"
#include "IMainFrameModule.h"
#include "ModuleManager.h"
#include "DesktopPlatformModule.h"
#include "ContentBrowserModule.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

/** 
* Shows a slate error message
* @param msg The error message to show
*/
void FAssetTypeActions_FaceFxBase::ShowError(const FText& msg)
{
	FNotificationInfo info(msg);
	info.ExpireDuration = 10.F;
	info.bUseLargeFont = false;
	info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
	FSlateNotificationManager::Get().AddNotification(info);
}

/** 
* Shows a slate info message
* @param msg The info message to show
*/
void FAssetTypeActions_FaceFxBase::ShowInfo(const FText& msg)
{
	FNotificationInfo info(msg);
	info.ExpireDuration = 10.F;
	info.bUseLargeFont = false;
	info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
	FSlateNotificationManager::Get().AddNotification(info);
}

/** Determine if we can recompile assets */
bool FAssetTypeActions_FaceFxBase::CanExecuteReimport(const TArray<UObject*> Objects) const
{
	return FFaceFxEditorTools::IsFaceFXCompilerInstalled();
}

/** Determine if we can set a new source */
bool FAssetTypeActions_FaceFxBase::CanExecuteSetSource(const TArray<UObject*> Objects) const
{
	return Objects.Num() == 1 && Objects[0]->IsA(UFaceFxAsset::StaticClass());
}

/** Determine if we can show details */
bool FAssetTypeActions_FaceFxBase::CanExecuteShowDetails(const TArray<UObject*> Objects) const
{
	if(Objects.Num() == 1)
	{
		if(const UFaceFxAsset* fxAsset = Cast<UFaceFxAsset>(Objects[0]))
		{
			return fxAsset->IsValid();
		}
	}
	return false;
}

/** Handler for when SetSource is selected */
void FAssetTypeActions_FaceFxBase::ExecuteSetSource(TArray<TWeakObjectPtr<UObject>> Objects)
{
	if(Objects.Num() == 0)
	{
		//failure
		ShowError(LOCTEXT("FaceFX_SetSourceFailedMissing","Missing asset."));
		return;
	}

	UFaceFxAsset* fxAsset = Cast<UFaceFxAsset>(Objects[0].Get());
	if(!fxAsset)
	{
		//failure
		ShowError(LOCTEXT("FaceFX_SetSourceFailedInvalid","Invalid asset type."));
		return;
	}

	if(IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get())
	{
		//get parent window
		void* parentWindowWindowHandle = nullptr;
		IMainFrameModule& mainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& mainFrameParentWindow = mainFrameModule.GetParentWindow();
		if ( mainFrameParentWindow.IsValid() && mainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			parentWindowWindowHandle = mainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		//query for files
		TArray<FString> files;
		desktopPlatform->OpenFileDialog(parentWindowWindowHandle, "Select FaceFX Asset", "", fxAsset->GetAssetPathAbsolute(), "FaceFX Asset (*.facefx)|*.facefx;", EFileDialogFlags::None, files);

		if(files.Num() <= 0)
		{
			//no file selected
			ShowError(LOCTEXT("FaceFX_SetSourceFailedCancelled", "FaceFX asset selection cancelled."));
			return;
		}

		//assign new file and reimport
		FString errorMsg;
		if(fxAsset->InitializeFromFaceFx(files[0], &errorMsg))
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("FaceFX_SetSourceSuccess", "Relink and compilation of FaceFX Set successfully completed. %s").ToString(), *errorMsg)));
		}
		else
		{
			//initialization failed
			ShowError(FText::FromString(FString::Printf(*LOCTEXT("FaceFX_SetSourceFailedInit", "Initialization of FaceFX Set failed. %s").ToString(), *errorMsg)));
		}
	}
	else
	{
		//failure
		ShowError(LOCTEXT("FaceFX_SetSourceFailedPlatform","Unable to fetch desktop platform module."));
	}
}

/** Handler for when Reimport is selected */
void FAssetTypeActions_FaceFxBase::ExecuteReimport(TArray<TWeakObjectPtr<UObject>> Objects)
{
	bool anyError = false;

	TArray<UFaceFxAsset*> succeedAssets;

	GWarn->BeginSlowTask(LOCTEXT("FaceFX_ReimportProgress", "Reimporting FaceFX Assets..."), true);
	int32 progress = 0;

	for (const TWeakObjectPtr<UObject>& object : Objects)
	{
		if(UFaceFxAsset* fxAsset = Cast<UFaceFxAsset>(object.Get()))
		{
			FCompilationBeforeDeletionDelegate deletionDelegate;
			if(fxAsset->IsA(UFaceFxActor::StaticClass()))
			{
				//actor assets may lead to changed animation sets
				deletionDelegate = FCompilationBeforeDeletionDelegate::CreateRaw(this, &FAssetTypeActions_FaceFxBase::OnReimportBeforeDelete);
			}

			FString errMsg;
			if(FFaceFxEditorTools::ReImportFaceFXAsset(fxAsset, &errMsg, deletionDelegate))
			{
				//success
				succeedAssets.Add(fxAsset);
			}
			else
			{
				//failure
				ShowError(FText::FromString(errMsg));
				anyError = true;
			}
		}

		GWarn->UpdateProgress(++progress, Objects.Num());
	}

	GWarn->EndSlowTask();

	if(!anyError)
	{
		if(succeedAssets.Num() > 1)
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("FaceFX_ReimportSuccess","Reimporting %i assets successfully completed.").ToString(), Objects.Num() )));
		}
		else if(succeedAssets.Num() == 1)
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("FaceFX_ReimportSuccessSingle","Reimporting asset successfully completed. (%i animations)").ToString(), succeedAssets[0]->GetAnimationCount())));
		}
	}
}

/** Callback function for before the compilation folder gets deleted */
void FAssetTypeActions_FaceFxBase::OnReimportBeforeDelete(class UObject* asset, const FString& compilationFolder, bool loadResult)
{
	if(loadResult)
	{
		UFaceFxActor* fxActor = CastChecked<UFaceFxActor>(asset);

		//asset successfully loaded -> get all anim groups and diff to existing asset
		TArray<FString> animGroups;
		if(FFaceFxEditorTools::GetAnimationGroupsInFolder(compilationFolder, EFaceFxTargetPlatform::PC, animGroups))
		{
			TArray<FName> actorAnimGroups;
			fxActor->GetAnimationGroups(actorAnimGroups);

			//collect all deleted groups
			FString addedGroupsS;
			FString deletedGroupsS;
			FString existingGroupsS;
			TArray<FString> deletedGroups;
			TArray<FString> addedGroups;
			TArray<FString> existingGroups;
			for(auto& group : animGroups)
			{
				if(!actorAnimGroups.Contains(FName(*group)))
				{
					addedGroups.Add(group);
					addedGroupsS += group + TEXT("\n");
				}
				else
				{
					existingGroups.Add(group);
					existingGroupsS += group + TEXT("\n");
				}
			}

			for(auto& group : actorAnimGroups)
			{
				const FString groupS = group.GetPlainNameString();
				if(!animGroups.Contains(groupS))
				{
					deletedGroups.Add(groupS);
					deletedGroupsS += groupS + TEXT("\n");
				}
			}

			if(addedGroups.Num() > 0)
			{
				//new anim sets available -> add for creation/link
				FFormatNamedArguments titleArgs;
				titleArgs.Add(TEXT("Asset"), FText::FromString(fxActor->GetName()));

				const FText title = FText::Format(LOCTEXT("ReimportFxActorAddedAnimationTitle", "Import Added Animation Groups [{Asset}]"), titleArgs);

				FFormatNamedArguments args;
				args.Add(TEXT("Groups"), FText::FromString(addedGroupsS));

				const EAppReturnType::Type result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFxActorAddedMessage", "Do you want to automatically import the unlinked animation groups and link them to this actor asset ?\n\nGroups:\n{Groups}"), args),
					&title);

				if(result == EAppReturnType::Yes)
				{
					FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
					auto& assetTools = assetToolsModule.Get();

					FString packageName;
					asset->GetOutermost()->GetName(packageName);

					for(auto& group : addedGroups)
					{
						if(!FFaceFxEditorTools::CreateAnimSetAsset(compilationFolder, group, packageName, fxActor, assetTools))
						{
							FFormatNamedArguments args;
							args.Add(TEXT("Asset"), FText::FromString(group));
							ShowError(FText::Format(LOCTEXT("AnimSetImportFailed", "FaceFX anim set import failed. Asset: {Asset}"), args));
						}
					}
				}
			}

			if(deletedGroups.Num() > 0)
			{
				//anim sets deleted -> ask for unlink
				FFormatNamedArguments titleArgs;
				titleArgs.Add(TEXT("Asset"), FText::FromString(fxActor->GetName()));

				const FText title = FText::Format(LOCTEXT("ReimportFxActorDeletedAnimationTitle", "Unlink Deleted Animation Sets [{Asset}]"), titleArgs);

				FFormatNamedArguments args;
				args.Add(TEXT("Groups"), FText::FromString(deletedGroupsS));

				const EAppReturnType::Type result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFxActorDeletedMessage", "There are animation groups which are removed from the FaceFx source. Do you want those to unlink from the actor asset ?\n\nGroups:\n{Groups}"), args),
					&title);

				if(result == EAppReturnType::Yes)
				{
					for(auto& group : deletedGroups)
					{
						if(auto asset = fxActor->GetAnimationSet(FName(*group)))
						{
							fxActor->UnlinkFrom(asset);
						}
					}
				}
			}

			if(existingGroups.Num() > 0)
			{
				//existing animation sets -> ask to reimport them as well
				FFormatNamedArguments titleArgs;
				titleArgs.Add(TEXT("Asset"), FText::FromString(fxActor->GetName()));

				const FText title = FText::Format(LOCTEXT("ReimportFxActorReimportAnimationTitle", "Reimport Animation Sets [{Asset}]"), titleArgs);

				FFormatNamedArguments args;
				args.Add(TEXT("Groups"), FText::FromString(existingGroupsS));

				const EAppReturnType::Type result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFxActorReimportMessage", "Do you want to reimport all linked animation sets ?\n\nGroups:\n{Groups}"), args),
					&title);

				if(result == EAppReturnType::Yes)
				{
					for(auto& group : existingGroups)
					{
						const FName groupName = FName(*group);

						if(auto asset = fxActor->GetAnimationSet(groupName))
						{
							FString errorMsg;
							if(asset->LoadFromFaceFxCompilationFolder(compilationFolder, &errorMsg))
							{
								FFaceFxEditorTools::SavePackage(asset->GetOutermost());
							}
							else
							{
								ShowError(FText::FromString(FString::Printf(*LOCTEXT("FaceFX_ReimportFaceFxAnimFailed", "Reimport of FaceFX Anim Set <%s> failed. %s").ToString(), *asset->GetName(), *errorMsg)));
							}
						}
					}
				}
			}
		}
	}
}

/** Handler for when ShowDetails is selected */
void FAssetTypeActions_FaceFxBase::ExecuteShowDetails(TArray<TWeakObjectPtr<UObject>> Objects)
{
	if(Objects.Num() != 1 || !GEditor)
	{
		return;
	}

	UFaceFxAsset* fxAsset = CastChecked<UFaceFxAsset>(Objects[0].Get());
	FString details;
	fxAsset->GetDetails(details);

	if(!details.IsEmpty())
	{
		OpenMsgDlgInt(EAppMsgType::Ok, FText::FromString(details), LOCTEXT("FaceFX_DetailsWindow", "FaceFX Asset Details"));
	}
}

#if PLATFORM_WINDOWS
/** Handler for when OpenFolder is selected */
void FAssetTypeActions_FaceFxBase::ExecuteOpenFolder(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FString errors;

	for (const TWeakObjectPtr<UObject>& object : Objects)
	{
		if(UFaceFxAsset* fxAsset = Cast<UFaceFxAsset>(object.Get()))
		{
			if(fxAsset->IsValid())
			{
				const FString pathAbs = fxAsset->GetAssetPathAbsolute();
				if(FPaths::FileExists(pathAbs))
				{
					//fire and forget
					FPlatformProcess::CreateProc(TEXT("explorer.exe"), *FString::Printf(TEXT("/select,\"%s\""), *pathAbs.Replace(TEXT("/"), TEXT("\\"))), true, false, false, nullptr, 0, nullptr, nullptr);
				}
				else
				{
					errors += FString::Printf(*LOCTEXT("FaceFX_OpenFolderMissing", "Asset does not exist: %s\n").ToString(), *pathAbs);
				}
			}
		}
	}

	if(!errors.IsEmpty())
	{
		//we have some errors
		ShowError(FText::FromString(errors));
	}
}

/** Determine if we can open the source asset folder */
bool FAssetTypeActions_FaceFxBase::CanExecuteOpenFolder(const TArray<UObject*> Objects) const
{
	for (const TWeakObjectPtr<UObject>& object : Objects)
	{
		if (UFaceFxAsset* fxAsset = Cast<UFaceFxAsset>(object.Get()))
		{
			if(fxAsset->IsAssetPathSet())
			{
				return true;
			}
		}
	}
	return false;
}

#endif