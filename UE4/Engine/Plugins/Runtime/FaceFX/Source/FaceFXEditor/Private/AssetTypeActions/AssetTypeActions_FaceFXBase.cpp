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
#include "FaceFX.h"
#include "AssetTypeActions_FaceFXBase.h"
#include "Factories/FaceFxActorFactory.h"
#include "Include/Slate/FaceFXResultWidget.h"

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
	return true;
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
		DesktopPlatform->OpenFileDialog(ParentWindowWindowHandle, LOCTEXT("OpenAssetTitle", "Select FaceFX Asset").ToString(), TEXT(""), FaceFXAsset->GetAssetPathAbsolute(), FACEFX_FILEFILTER_ASSET_ACTOR, EFileDialogFlags::None, Files);

		if(Files.Num() <= 0)
		{
			//no file selected
			ShowError(LOCTEXT("SetSourceFailedCancelled", "FaceFX asset selection cancelled."));
			return;
		}

		if(UFaceFXAnim* FaceFXAnim = Cast<UFaceFXAnim>(FaceFXAsset))
		{
			//remove previous animation id in case of an UFaceFXAnim so the init phase will ask the user for an animation of the new source
			FaceFXAnim->GetId().Reset();
		}		

		//assign new file and reimport
		FFaceFXImportResultSet ResultSet;
		FFaceFXEditorTools::InitializeFromFile(FaceFXAsset, Files[0], ResultSet.GetOrAdd(FaceFXAsset));

		FFaceFXResultWidget::Create(LOCTEXT("ShowSetSourceResultTitle", "Set Source Result"), ResultSet);
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
	TArray<UFaceFXAsset*> SucceedAssets;

	GWarn->BeginSlowTask(LOCTEXT("ReimportProgress", "Reimporting FaceFX Assets..."), true);
	int32 progress = 0;

	//The whole result set
	FFaceFXImportResultSet ResultSet;

	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if(UFaceFXAsset* FaceFXAsset = Cast<UFaceFXAsset>(Object.Get()))
		{
			FFaceFXImportResult& Result = ResultSet.GetOrAdd(FaceFXAsset);
			
			FCompilationBeforeDeletionDelegate DeletionDelegate;
			if(FaceFXAsset->IsA(UFaceFXActor::StaticClass()) && FFaceFXEditorTools::IsImportAnimationOnActorImport())
			{
				//actor assets may lead to changed animation sets
				DeletionDelegate = FCompilationBeforeDeletionDelegate::CreateRaw(this, &FAssetTypeActions_FaceFXBase::OnReimportBeforeDelete);
			}

			FFaceFXEditorTools::ReImportFaceFXAsset(FaceFXAsset, Result, DeletionDelegate);
		}

		GWarn->UpdateProgress(++progress, Objects.Num());
	}

	GWarn->EndSlowTask();

	FFaceFXResultWidget::Create(LOCTEXT("ShowReimportResultTitle", "Reimport Result"), ResultSet);
}

void FAssetTypeActions_FaceFXBase::OnReimportBeforeDelete(class UObject* Asset, const FString& CompilationFolder, bool LoadResult, FFaceFXImportResult& OutResultMessages)
{
	if(LoadResult)
	{
		UFaceFXActorFactory::HandleFaceFXActorCreated(CastChecked<UFaceFXActor>(Asset), CompilationFolder, OutResultMessages);
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