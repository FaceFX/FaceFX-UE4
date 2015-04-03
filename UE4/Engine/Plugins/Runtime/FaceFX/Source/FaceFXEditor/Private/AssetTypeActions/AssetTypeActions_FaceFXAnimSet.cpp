#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "AssetTypeActions_FaceFXAnimSet.h"

#include "UnrealEd.h"
#include "IMainFrameModule.h"
#include "ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FaceFX"

FText FAssetTypeActions_FaceFXAnimSet::GetName() const 
{ 
	return LOCTEXT("AssetTypeActions_FaceFXAnimSet", "FaceFX Anim Set"); 
}

UClass* FAssetTypeActions_FaceFXAnimSet::GetSupportedClass() const 
{ 
	return UFaceFXAnimSet::StaticClass();
}

void FAssetTypeActions_FaceFXAnimSet::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
#if PLATFORM_WINDOWS
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_OpenFolder","Open FaceFX Studio Asset Folder"),
		LOCTEXT("Asset_OpenFolderTooltip", "Opens the folder where the FaceFX Studio Asset is located in."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteOpenFolder, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXAnimSet::CanExecuteOpenFolder, InObjects)
		)
		);
#endif

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Reimport","Reimport FaceFX Studio Assets"),
		LOCTEXT("Asset_ReimportTooltip", "Reimports the assets which were exported from FaceFX Studio for this asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteReimport, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXAnimSet::CanExecuteReimport, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_SetSource","Set new FaceFX Studio Assets"),
		LOCTEXT("Asset_SetSourceTooltip", "Selects a new FaceFX Studio source for this asset and reimports the new one."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteSetSource, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXAnimSet::CanExecuteSetSource, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_ShowDetails","Show FaceFX Asset Details"),
		LOCTEXT("Asset_ShowDetailsTooltip", "Shows the details of the selected FaceFX asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteShowDetails, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFXAnimSet::CanExecuteShowDetails, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_LinkFXAnim","Link With FaceFX Actor"),
		LOCTEXT("Asset_LinkFXAnimTooltip", "Links this asset with existing FaceFX Actor assets."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteLink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_UnlinkFXAnim","Unlink From FaceFX Actor"),
		LOCTEXT("Asset_UnlinkFXAnimTooltip", "Unlinks this asset from existing FaceFX Actor assets."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFXAnimSet::ExecuteUnlink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);
}

void FAssetTypeActions_FaceFXAnimSet::ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig OpenConfig;
	OpenConfig.DialogTitleOverride = LOCTEXT("Asset_LinkFXAnimSelectTitle","Select Actor Asset To Link With");
	OpenConfig.AssetClassNames.Add(UFaceFXActor::StaticClass()->GetFName());
	OpenConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().CreateOpenAssetDialog(OpenConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFXAnimSet::OnAssetLinkChosen, Objects), FOnAssetDialogCancelled());
}

void FAssetTypeActions_FaceFXAnimSet::ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig OpenConfig;
	OpenConfig.DialogTitleOverride = LOCTEXT("Asset_UnlinkFXAnimSelectTitle","Select Actor Asset To Unlink From");
	OpenConfig.AssetClassNames.Add(UFaceFXActor::StaticClass()->GetFName());
	OpenConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().CreateOpenAssetDialog(OpenConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFXAnimSet::OnAssetUnlinkChosen, Objects), FOnAssetDialogCancelled());
}

void FAssetTypeActions_FaceFXAnimSet::OnAssetLinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	if(SelectedAssets.Num() == 0 || SelectedObjects.Num() == 0)
	{
		return;
	}

	for(const FAssetData& Asset : SelectedAssets)
	{
		bool bHasChanged = false;

		if(UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Asset.GetAsset()))
		{
			for(auto& Object : SelectedObjects)
			{
				if(UFaceFXAnimSet* AnimSet = Cast<UFaceFXAnimSet>(Object.Get()))
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

void FAssetTypeActions_FaceFXAnimSet::OnAssetUnlinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	if(SelectedAssets.Num() == 0 || SelectedObjects.Num() == 0)
	{
		return;
	}

	for(const FAssetData& Asset : SelectedAssets)
	{
		bool bHasChanged = false;

		if(UFaceFXActor* FaceFXActor = Cast<UFaceFXActor>(Asset.GetAsset()))
		{
			for(auto& Object : SelectedObjects)
			{
				if(UFaceFXAnimSet* AnimSet = Cast<UFaceFXAnimSet>(Object.Get()))
				{
					FaceFXActor->UnlinkFrom(AnimSet);
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

#undef LOCTEXT_NAMESPACE
