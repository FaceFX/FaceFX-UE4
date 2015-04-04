#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "AssetTypeActions_FaceFXBase.h"

#include "UnrealEd.h"
#include "MainFrame.h"
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
void FAssetTypeActions_FaceFXBase::ShowError(const FText& msg)
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
void FAssetTypeActions_FaceFXBase::ShowInfo(const FText& msg)
{
	FNotificationInfo info(msg);
	info.ExpireDuration = 10.F;
	info.bUseLargeFont = false;
	info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
	FSlateNotificationManager::Get().AddNotification(info);
}

/** Determine if we can recompile assets */
bool FAssetTypeActions_FaceFXBase::CanExecuteReimport(const TArray<UObject*> Objects) const
{
	return FFaceFXEditorTools::IsFaceFXCompilerInstalled();
}

/** Determine if we can set a new source */
bool FAssetTypeActions_FaceFXBase::CanExecuteSetSource(const TArray<UObject*> Objects) const
{
	return Objects.Num() == 1 && Objects[0]->IsA(UFaceFXAsset::StaticClass());
}

/** Determine if we can show details */
bool FAssetTypeActions_FaceFXBase::CanExecuteShowDetails(const TArray<UObject*> Objects) const
{
	if(Objects.Num() == 1)
	{
		if(const UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Objects[0]))
		{
			return FaceFXAsset->IsValid();
		}
	}
	return false;
}

/** Handler for when SetSource is selected */
void FAssetTypeActions_FaceFXBase::ExecuteSetSource(TArray<TWeakObjectPtr<UObject>> Objects)
{
	if(Objects.Num() == 0)
	{
		//failure
		ShowError(LOCTEXT("SetSourceFailedMissing","Missing asset."));
		return;
	}

	UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Objects[0].Get());
	if(!FaceFXAsset)
	{
		//failure
		ShowError(LOCTEXT("SetSourceFailedInvalid","Invalid asset type."));
		return;
	}

	if(IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
	{
		//get parent window
		void* ParentWindowWindowHandle = nullptr;
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		//query for files
		TArray<FString> Files;
		DesktopPlatform->OpenFileDialog(ParentWindowWindowHandle, "Select FaceFX Asset", "", FaceFXAsset->GetAssetPathAbsolute(), "FaceFX Asset (*.facefx)|*.facefx;", EFileDialogFlags::None, Files);

		if(Files.Num() <= 0)
		{
			//no file selected
			ShowError(LOCTEXT("SetSourceFailedCancelled", "FaceFX asset selection cancelled."));
			return;
		}

		//assign new file and reimport
		FString ErrorMsg;
		if(FFaceFXEditorTools::InitializeFromFile(FaceFXAsset, Files[0], &ErrorMsg))
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("SetSourceSuccess", "Relink and compilation of FaceFX Set successfully completed. %s").ToString(), *ErrorMsg)));
		}
		else
		{
			//initialization failed
			ShowError(FText::FromString(FString::Printf(*LOCTEXT("SetSourceFailedInit", "Initialization of FaceFX Set failed. %s").ToString(), *ErrorMsg)));
		}
	}
	else
	{
		//failure
		ShowError(LOCTEXT("SetSourceFailedPlatform","Unable to fetch desktop platform module."));
	}
}

/** Handler for when Reimport is selected */
void FAssetTypeActions_FaceFXBase::ExecuteReimport(TArray<TWeakObjectPtr<UObject>> Objects)
{
	bool bAnyError = false;

	TArray<UFaceFXAsset*> SucceedAssets;

	GWarn->BeginSlowTask(LOCTEXT("ReimportProgress", "Reimporting FaceFX Assets..."), true);
	int32 progress = 0;

	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if(UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Object.Get()))
		{
			FCompilationBeforeDeletionDelegate DeletionDelegate;
			if(FaceFXAsset->IsA(UFaceFXActor::StaticClass()))
			{
				//actor assets may lead to changed animation sets
				DeletionDelegate = FCompilationBeforeDeletionDelegate::CreateRaw(this, &FAssetTypeActions_FaceFXBase::OnReimportBeforeDelete);
			}

			FString ErrMsg;
			if(FFaceFXEditorTools::ReImportFaceFXAsset(FaceFXAsset, &ErrMsg, DeletionDelegate))
			{
				//success
				SucceedAssets.Add(FaceFXAsset);
			}
			else
			{
				//failure
				ShowError(FText::FromString(ErrMsg));
				bAnyError = true;
			}
		}

		GWarn->UpdateProgress(++progress, Objects.Num());
	}

	GWarn->EndSlowTask();

	if(!bAnyError)
	{
		if(SucceedAssets.Num() > 1)
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("ReimportSuccess","Reimporting %i assets successfully completed.").ToString(), Objects.Num() )));
		}
		else if(SucceedAssets.Num() == 1)
		{
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("ReimportSuccessSingle","Reimporting asset successfully completed. (%i animations)").ToString(), SucceedAssets[0]->GetAnimationCount())));
		}
	}
}

void FAssetTypeActions_FaceFXBase::OnReimportBeforeDelete(class UObject* Asset, const FString& CompilationFolder, bool LoadResult)
{
	if(LoadResult)
	{
		UFaceFXActor* FaceFXActor = CastChecked<UFaceFXActor>(Asset);

		//asset successfully loaded -> get all anim groups and diff to existing asset
		TArray<FString> AnimGroups;
		if(FFaceFXEditorTools::GetAnimationGroupsInFolder(CompilationFolder, EFaceFXTargetPlatform::PC, AnimGroups))
		{
			TArray<FName> ActorAnimGroups;
			FaceFXActor->GetAnimationGroups(ActorAnimGroups);

			//collect all deleted groups
			FString AddedGroupsS;
			FString DeletedGroupsS;
			FString ExistingGroupsS;
			TArray<FString> DeletedGroups;
			TArray<FString> AddedGroups;
			TArray<FString> ExistingGroups;
			for(auto& Group : AnimGroups)
			{
				if(!ActorAnimGroups.Contains(FName(*Group)))
				{
					AddedGroups.Add(Group);
					AddedGroupsS += Group + TEXT("\n");
				}
				else
				{
					ExistingGroups.Add(Group);
					ExistingGroupsS += Group + TEXT("\n");
				}
			}

			for(auto& Group : ActorAnimGroups)
			{
				const FString GroupS = Group.GetPlainNameString();
				if(!AnimGroups.Contains(GroupS))
				{
					DeletedGroups.Add(GroupS);
					DeletedGroupsS += GroupS + TEXT("\n");
				}
			}

			if(AddedGroups.Num() > 0)
			{
				//new anim sets available -> add for creation/link
				FFormatNamedArguments TitleArgs;
				TitleArgs.Add(TEXT("Asset"), FText::FromString(FaceFXActor->GetName()));

				const FText Title = FText::Format(LOCTEXT("ReimportFXActorAddedAnimationTitle", "Import Added Animation Groups [{Asset}]"), TitleArgs);

				FFormatNamedArguments Args;
				Args.Add(TEXT("Groups"), FText::FromString(AddedGroupsS));

				const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFXActorAddedMessage", "Do you want to automatically import the unlinked animation groups and link them to this actor asset ?\n\nGroups:\n{Groups}"), Args),
					&Title);

				if(Result == EAppReturnType::Yes)
				{
					FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
					auto& AssetTools = AssetToolsModule.Get();

					FString PackageName;
					Asset->GetOutermost()->GetName(PackageName);

					for(auto& Group : AddedGroups)
					{
						if(!FFaceFXEditorTools::CreateAnimSetAsset(CompilationFolder, Group, PackageName, FaceFXActor, AssetTools))
						{
							FFormatNamedArguments Args;
							Args.Add(TEXT("Asset"), FText::FromString(Group));
							ShowError(FText::Format(LOCTEXT("AnimSetImportFailed", "FaceFX anim set import failed. Asset: {Asset}"), Args));
						}
					}
				}
			}

			if(DeletedGroups.Num() > 0)
			{
				//anim sets deleted -> ask for unlink
				FFormatNamedArguments TitleArgs;
				TitleArgs.Add(TEXT("Asset"), FText::FromString(FaceFXActor->GetName()));

				const FText Title = FText::Format(LOCTEXT("ReimportFXActorDeletedAnimationTitle", "Unlink Deleted Animation Sets [{Asset}]"), TitleArgs);

				FFormatNamedArguments Args;
				Args.Add(TEXT("Groups"), FText::FromString(DeletedGroupsS));

				const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFXActorDeletedMessage", "There are animation groups which are removed from the FaceFX source. Do you want those to unlink from the actor asset ?\n\nGroups:\n{Groups}"), Args),
					&Title);

				if(Result == EAppReturnType::Yes)
				{
					for(auto& Group : DeletedGroups)
					{
						if(auto Asset = FaceFXActor->GetAnimationSet(FName(*Group)))
						{
							FaceFXActor->UnlinkFrom(Asset);
						}
					}
				}
			}

			if(ExistingGroups.Num() > 0)
			{
				//existing animation sets -> ask to reimport them as well
				FFormatNamedArguments TitleArgs;
				TitleArgs.Add(TEXT("Asset"), FText::FromString(FaceFXActor->GetName()));

				const FText Title = FText::Format(LOCTEXT("ReimportFXActorReimportAnimationTitle", "Reimport Animation Sets [{Asset}]"), TitleArgs);

				FFormatNamedArguments Args;
				Args.Add(TEXT("Groups"), FText::FromString(ExistingGroupsS));

				const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ReimportFXActorReimportMessage", "Do you want to reimport all linked animation sets ?\n\nGroups:\n{Groups}"), Args),
					&Title);

				if(Result == EAppReturnType::Yes)
				{
					for(auto& Group : ExistingGroups)
					{
						const FName GroupName = FName(*Group);

						if(auto Asset = FaceFXActor->GetAnimationSet(GroupName))
						{
							FString ErrorMsg;
							if(FFaceFXEditorTools::LoadFromCompilationFolder(Asset, CompilationFolder, &ErrorMsg))
							{
								FFaceFXEditorTools::SavePackage(Asset->GetOutermost());
							}
							else
							{
								ShowError(FText::FromString(FString::Printf(*LOCTEXT("ReimportFaceFXAnimFailed", "Reimport of FaceFX Anim Set <%s> failed. %s").ToString(), *Asset->GetName(), *ErrorMsg)));
							}
						}
					}
				}
			}
		}
	}
}

/** Handler for when ShowDetails is selected */
void FAssetTypeActions_FaceFXBase::ExecuteShowDetails(TArray<TWeakObjectPtr<UObject>> Objects)
{
	if(Objects.Num() != 1 || !GEditor)
	{
		return;
	}

	UFaceFXAsset* FaceFXAsset = CastChecked<UFaceFXAsset>(Objects[0].Get());
	FString Details;
	FaceFXAsset->GetDetails(Details);

	if(!Details.IsEmpty())
	{
		OpenMsgDlgInt(EAppMsgType::Ok, FText::FromString(Details), LOCTEXT("DetailsWindowTitle", "FaceFX Asset Details"));
	}
}

#if PLATFORM_WINDOWS
/** Handler for when OpenFolder is selected */
void FAssetTypeActions_FaceFXBase::ExecuteOpenFolder(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FString Errors;

	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if(UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Object.Get()))
		{
			if(FaceFXAsset->IsValid())
			{
				const FString PathAbs = FaceFXAsset->GetAssetPathAbsolute();
				if(FPaths::FileExists(PathAbs))
				{
					//fire and forget
					FPlatformProcess::CreateProc(TEXT("explorer.exe"), *FString::Printf(TEXT("/select,\"%s\""), *PathAbs.Replace(TEXT("/"), TEXT("\\"))), true, false, false, nullptr, 0, nullptr, nullptr);
				}
				else
				{
					Errors += FString::Printf(*LOCTEXT("OpenFolderMissing", "Asset does not exist: %s\n").ToString(), *PathAbs);
				}
			}
		}
	}

	if(!Errors.IsEmpty())
	{
		//we have some errors
		ShowError(FText::FromString(Errors));
	}
}

/** Determine if we can open the source asset folder */
bool FAssetTypeActions_FaceFXBase::CanExecuteOpenFolder(const TArray<UObject*> Objects) const
{
	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if (UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Object.Get()))
		{
			if(FaceFXAsset->IsAssetPathSet())
			{
				return true;
			}
		}
	}
	return false;
}

#endif