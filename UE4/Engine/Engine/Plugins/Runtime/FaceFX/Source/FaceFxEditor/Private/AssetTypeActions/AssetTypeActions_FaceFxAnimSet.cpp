#include "FaceFxEditor.h"
#include "FaceFx.h"
#include "AssetTypeActions_FaceFxAnimSet.h"

#include "UnrealEd.h"
#include "IMainFrameModule.h"
#include "ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FaceFx"

UClass* FAssetTypeActions_FaceFxAnimSet::GetSupportedClass() const 
{ 
	return UFaceFxAnimSet::StaticClass();
}

void FAssetTypeActions_FaceFxAnimSet::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
#if PLATFORM_WINDOWS
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_OpenFolder","Open FaceFX Studio Asset Folder"),
		LOCTEXT("Asset_OpenFolderTooltip", "Opens the folder where the FaceFX Studio Asset is located in."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteOpenFolder, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxAnimSet::CanExecuteOpenFolder, InObjects)
		)
		);
#endif

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Reimport","Reimport FaceFX Studio Assets"),
		LOCTEXT("Asset_ReimportTooltip", "Reimports the assets which were exported from FaceFX Studio for this asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteReimport, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxAnimSet::CanExecuteReimport, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_SetSource","Set new FaceFX Studio Assets"),
		LOCTEXT("Asset_SetSourceTooltip", "Selects a new FaceFX Studio source for this asset and reimports the new one."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteSetSource, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxAnimSet::CanExecuteSetSource, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_ShowDetails","Show FaceFX Asset Details"),
		LOCTEXT("Asset_ShowDetailsTooltip", "Shows the details of the selected FaceFX asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteShowDetails, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxAnimSet::CanExecuteShowDetails, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_LinkFxAnim","Link With FaceFX Actor"),
		LOCTEXT("Asset_LinkFxAnimTooltip", "Links this asset with existing FaceFX Actor assets."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteLink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_UnlinkFxAnim","Unlink From FaceFX Actor"),
		LOCTEXT("Asset_UnlinkFxAnimTooltip", "Unlinks this asset from existing FaceFX Actor assets."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxAnimSet::ExecuteUnlink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);
}

/** Handler for when Link is selected */
void FAssetTypeActions_FaceFxAnimSet::ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig openConfig;
	openConfig.DialogTitleOverride = LOCTEXT("Asset_LinkFxAnimSelectTitle","Select Actor Asset To Link With");
	openConfig.AssetClassNames.Add(UFaceFxActor::StaticClass()->GetFName());
	openConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	contentBrowserModule.Get().CreateOpenAssetDialog(openConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFxAnimSet::OnAssetLinkChosen, Objects), FOnAssetDialogCancelled());
}

/** Handler for when Unlink is selected */
void FAssetTypeActions_FaceFxAnimSet::ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig openConfig;
	openConfig.DialogTitleOverride = LOCTEXT("Asset_UnlinkFxAnimSelectTitle","Select Actor Asset To Unlink From");
	openConfig.AssetClassNames.Add(UFaceFxActor::StaticClass()->GetFName());
	openConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	contentBrowserModule.Get().CreateOpenAssetDialog(openConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFxAnimSet::OnAssetUnlinkChosen, Objects), FOnAssetDialogCancelled());
}

/** Callback for when the assets to link with have been chosen for a set of objects */
void FAssetTypeActions_FaceFxAnimSet::OnAssetLinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects)
{
	if(selectedAssets.Num() == 0 || selectedObjects.Num() == 0)
	{
		return;
	}

	for(const FAssetData& asset : selectedAssets)
	{
		bool hasChanged = false;

		if(UFaceFxActor* fxActor = Cast<UFaceFxActor>(asset.GetAsset()))
		{
			for(auto& object : selectedObjects)
			{
				if(UFaceFxAnimSet* animSet = Cast<UFaceFxAnimSet>(object.Get()))
				{
					fxActor->LinkTo(animSet);
					hasChanged = true;
				}
			}

			if(hasChanged)
			{
				fxActor->GetOutermost()->MarkPackageDirty();
			}
		}
	}
}

/** Callback for when the assets to unlink with have been chosen for a set of objects */
void FAssetTypeActions_FaceFxAnimSet::OnAssetUnlinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects)
{
	if(selectedAssets.Num() == 0 || selectedObjects.Num() == 0)
	{
		return;
	}

	for(const FAssetData& asset : selectedAssets)
	{
		bool hasChanged = false;

		if(UFaceFxActor* fxActor = Cast<UFaceFxActor>(asset.GetAsset()))
		{
			for(auto& object : selectedObjects)
			{
				if(UFaceFxAnimSet* animSet = Cast<UFaceFxAnimSet>(object.Get()))
				{
					fxActor->UnlinkFrom(animSet);
					hasChanged = true;
				}
			}

			if(hasChanged)
			{
				fxActor->GetOutermost()->MarkPackageDirty();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
