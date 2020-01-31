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

#include "AssetTypeActions_FaceFXActor.h"
#include "FaceFXEditor.h"
#include "FaceFXEditorTools.h"
#include "FaceFX.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiboxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FaceFX"

FText FAssetTypeActions_FaceFXActor::GetName() const
{
	return LOCTEXT("AssetTypeActions_FaceFX", "FaceFX Actor");
}

UClass* FAssetTypeActions_FaceFXActor::GetSupportedClass() const
{
	return UFaceFXActor::StaticClass();
}

void FAssetTypeActions_FaceFXActor::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
#if PLATFORM_WINDOWS
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Edit","Open in FaceFX Studio"),
		LOCTEXT("Asset_EditTooltip", "Opens the Face FX asset within FaceFX studio."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteEdit, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXActor::CanExecuteEdit, InObjects)
		)
	);
#endif

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_OpenFolder","Open FaceFX Studio Asset Folder"),
		LOCTEXT("Asset_OpenFolderTooltip", "Opens the folder where the FaceFX Studio Asset is located in."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteOpenFolder, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXActor::CanExecuteOpenFolder, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Reimport","Reimport FaceFX Studio Assets"),
		LOCTEXT("Asset_ReimportTooltip", "Reimports the assets which were exported from FaceFX Studio for this asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteReimport, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXActor::CanExecuteReimport, InObjects)
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_SetSource","Set New FaceFX Studio Assets"),
		LOCTEXT("Asset_SetSourceTooltip", "Selects a new FaceFX Studio source for this asset and reimports the new one."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteSetSource, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXActor::CanExecuteSetSource, InObjects)
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_ShowDetails","Show FaceFX Asset Details"),
		LOCTEXT("Asset_ShowDetailsTooltip", "Shows the details of the selected FaceFX asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteShowDetails, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXActor::CanExecuteShowDetails, InObjects)
		)
		);

#if FACEFX_USEANIMATIONLINKAGE
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_LinkFXActor","Link FaceFX Anim Set"),
		LOCTEXT("Asset_LinkFXActorTooltip", "Links an existing FaceFX anim set asset with this FaceFX actor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteLink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_UnlinkFXActor","Unlink FaceFX Anim Set"),
		LOCTEXT("Asset_UnlinkFXActorTooltip", "Unlinks an existing FaceFX anim set asset from this FaceFX actor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXActor::ExecuteUnlink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);
#endif //FACEFX_USEANIMATIONLINKAGE
}

bool FAssetTypeActions_FaceFXActor::CanExecuteEdit(const TArray<UObject*> Objects) const
{
	return FFaceFXEditorTools::IsFaceFXStudioInstalled();
}

void FAssetTypeActions_FaceFXActor::ExecuteEdit(TArray< TWeakObjectPtr<UObject> > Objects)
{
	TArray<UObject*> Items;

	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if (UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Object.Get()))
		{
			Items.Add(FaceFXActor);
		}
	}

	if(Items.Num() > 0)
	{
		OpenAssetEditor(Items, nullptr);
	}
}

#if FACEFX_USEANIMATIONLINKAGE

void FAssetTypeActions_FaceFXActor::ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig OpenConfig;
	OpenConfig.DialogTitleOverride = LOCTEXT("Asset_LinkFXActorSelectTitle","Select Anim Set Asset To Link");
	OpenConfig.AssetClassNames.Add(UFaceFXAnim::StaticClass()->GetFName());
	OpenConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().CreateOpenAssetDialog(OpenConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFXActor::OnAssetLinkChosen, Objects), FOnAssetDialogCancelled());
}

void FAssetTypeActions_FaceFXActor::OnAssetLinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	if(SelectedAssets.Num() == 0 || SelectedObjects.Num() == 0)
	{
		return;
	}

	for(auto& Object : SelectedObjects)
	{
		if(UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Object.Get()))
		{
			bool bHasChanged = false;

			for(const FAssetData& Asset : SelectedAssets)
			{
				if(UFaceFXAnim* AnimSet = Cast<UFaceFXAnim>(Asset.GetAsset()))
				{
					FaceFXActor->LinkTo(AnimSet);
					bHasChanged = true;
				}
			}

			if(bHasChanged)
			{
				FaceFXActor->GetOutermost()->MarkPackageDirty();
			}
		}
	}
}

/** Handler for when Unlink is selected */
void FAssetTypeActions_FaceFXActor::ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig openConfig;
	openConfig.DialogTitleOverride = LOCTEXT("Asset_UnlinkFXActorSelectTitle","Select Anim Set Asset To Unlink from");
	openConfig.AssetClassNames.Add(UFaceFXAnim::StaticClass()->GetFName());
	openConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	contentBrowserModule.Get().CreateOpenAssetDialog(openConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFXActor::OnAssetUnlinkChosen, Objects), FOnAssetDialogCancelled());
}

/** Callback for when the assets to unlink with have been chosen for a set of objects */
void FAssetTypeActions_FaceFXActor::OnAssetUnlinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects)
{
	if(selectedAssets.Num() == 0 || selectedObjects.Num() == 0)
	{
		return;
	}

	for(auto& Object : selectedObjects)
	{
		if(UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Object.Get()))
		{
			bool bHasChanged = false;

			for(const FAssetData& Asset : selectedAssets)
			{
				if(UFaceFXAnim* AnimSet = Cast<UFaceFXAnim>(Asset.GetAsset()))
				{
					bHasChanged |= FaceFXActor->UnlinkFrom(AnimSet);
				}
			}

			if(bHasChanged)
			{
				FaceFXActor->GetOutermost()->MarkPackageDirty();
			}
		}
	}
}

#endif //FACEFX_USEANIMATIONLINKAGE

void FAssetTypeActions_FaceFXActor::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	bool bAnyError = false;

	for (UObject* Object : InObjects)
	{
		if(UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Object))
		{
			FString ErrMsg;
			if(!FFaceFXEditorTools::OpenFaceFXStudio(FaceFXActor, &ErrMsg))
			{
				FFaceFXEditorTools::ShowError(FText::FromString(ErrMsg));
				bAnyError = true;
			}
		}
	}

	if(!bAnyError)
	{
		FFaceFXEditorTools::ShowInfo(LOCTEXT("Asset_OpenAsset","Opening asset(s) in background process successfully completed."));
	}
}

#undef LOCTEXT_NAMESPACE
