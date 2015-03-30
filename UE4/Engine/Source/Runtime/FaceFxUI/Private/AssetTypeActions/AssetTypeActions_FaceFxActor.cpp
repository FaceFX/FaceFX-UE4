#include "FaceFxUI.h"
#include "FaceFx.h"
#include "AssetTypeActions_FaceFxActor.h"

#include "UnrealEd.h"
#include "IMainFrameModule.h"
#include "ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FaceFx"

UClass* FAssetTypeActions_FaceFxActor::GetSupportedClass() const 
{ 
	return UFaceFxActor::StaticClass();
}

void FAssetTypeActions_FaceFxActor::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Edit","Open in FaceFX Studio"),
		LOCTEXT("Asset_EditTooltip", "Opens the Face FX asset within FaceFX studio."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteEdit, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxActor::CanExecuteEdit, InObjects)
		)
	);

#if PLATFORM_WINDOWS
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_OpenFolder","Open FaceFX Studio Asset Folder"),
		LOCTEXT("Asset_OpenFolderTooltip", "Opens the folder where the FaceFX Studio Asset is located in."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteOpenFolder, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxActor::CanExecuteOpenFolder, InObjects)
		)
		);
#endif

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_Reimport","Reimport FaceFX Studio Assets"),
		LOCTEXT("Asset_ReimportTooltip", "Reimports the assets which were exported from FaceFX Studio for this asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteReimport, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxActor::CanExecuteReimport, InObjects)
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_SetSource","Set New FaceFX Studio Assets"),
		LOCTEXT("Asset_SetSourceTooltip", "Selects a new FaceFX Studio source for this asset and reimports the new one."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteSetSource, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxActor::CanExecuteSetSource, InObjects)
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_ShowDetails","Show FaceFX Asset Details"),
		LOCTEXT("Asset_ShowDetailsTooltip", "Shows the details of the selected FaceFX asset."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteShowDetails, GetTypedWeakObjectPtrs<UObject>(InObjects) ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_FaceFxActor::CanExecuteShowDetails, InObjects)
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_LinkFxActor","Link FaceFX Anim Set"),
		LOCTEXT("Asset_LinkFxActorTooltip", "Links an existing FaceFX anim set asset with this FaceFX actor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteLink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Asset_UnlinkFxActor","Unlink FaceFX Anim Set"),
		LOCTEXT("Asset_UnlinkFxActorTooltip", "Unlinks an existing FaceFX anim set asset from this FaceFX actor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_FaceFxActor::ExecuteUnlink, GetTypedWeakObjectPtrs<UObject>(InObjects) )
		)
		);
}

/** Determine if we can edit assets */
bool FAssetTypeActions_FaceFxActor::CanExecuteEdit(const TArray<UObject*> Objects) const
{
	return FFaceFxEditorTools::IsFaceFXStudioInstalled();
}

void FAssetTypeActions_FaceFxActor::ExecuteEdit(TArray< TWeakObjectPtr<UObject> > Objects)
{
	TArray<UObject*> items;

	for (const TWeakObjectPtr<UObject>& object : Objects)
	{
		if (UFaceFxActor* fxActor = Cast<UFaceFxActor>(object.Get()))
		{
			items.Add(fxActor);
		}
	}

	if(items.Num() > 0)
	{
		OpenAssetEditor(items, nullptr);
	}
}

/** Handler for when ShowDetails is selected */
void FAssetTypeActions_FaceFxActor::ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig openConfig;
	openConfig.DialogTitleOverride = LOCTEXT("Asset_LinkFxActorSelectTitle","Select Anim Set Asset To Link");
	openConfig.AssetClassNames.Add(UFaceFxAnimSet::StaticClass()->GetFName());
	openConfig.bAllowMultipleSelection = true;
	
	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	contentBrowserModule.Get().CreateOpenAssetDialog(openConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFxActor::OnAssetLinkChosen, Objects), FOnAssetDialogCancelled());
}

/** Callback for when the assets to link with have been chosen for a set of objects */
void FAssetTypeActions_FaceFxActor::OnAssetLinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects)
{
	if(selectedAssets.Num() == 0 || selectedObjects.Num() == 0)
	{
		return;
	}

	for(auto& object : selectedObjects)
	{
		if(UFaceFxActor* fxActor = Cast<UFaceFxActor>(object.Get()))
		{
			bool hasChanged = false;

			for(const FAssetData& asset : selectedAssets)
			{
				if(UFaceFxAnimSet* animSet = Cast<UFaceFxAnimSet>(asset.GetAsset()))
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

/** Handler for when Unlink is selected */
void FAssetTypeActions_FaceFxActor::ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects)
{
	FOpenAssetDialogConfig openConfig;
	openConfig.DialogTitleOverride = LOCTEXT("Asset_UnlinkFxActorSelectTitle","Select Anim Set Asset To Unlink from");
	openConfig.AssetClassNames.Add(UFaceFxAnimSet::StaticClass()->GetFName());
	openConfig.bAllowMultipleSelection = true;

	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	contentBrowserModule.Get().CreateOpenAssetDialog(openConfig, FOnAssetsChosenForOpen::CreateStatic(&FAssetTypeActions_FaceFxActor::OnAssetUnlinkChosen, Objects), FOnAssetDialogCancelled());
}

/** Callback for when the assets to unlink with have been chosen for a set of objects */
void FAssetTypeActions_FaceFxActor::OnAssetUnlinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects)
{
	if(selectedAssets.Num() == 0 || selectedObjects.Num() == 0)
	{
		return;
	}

	for(auto& object : selectedObjects)
	{
		if(UFaceFxActor* fxActor = Cast<UFaceFxActor>(object.Get()))
		{
			bool hasChanged = false;

			for(const FAssetData& asset : selectedAssets)
			{
				if(UFaceFxAnimSet* animSet = Cast<UFaceFxAnimSet>(asset.GetAsset()))
				{
					hasChanged |= fxActor->UnlinkFrom(animSet);
				}
			}

			if(hasChanged)
			{
				fxActor->GetOutermost()->MarkPackageDirty();
			}
		}
	}
}

void FAssetTypeActions_FaceFxActor::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	bool anyError = false;

	for (UObject* object : InObjects)
	{
		if(UFaceFxActor* fxActor = Cast<UFaceFxActor>(object))
		{
			FString errMsg;
			if(!FFaceFxEditorTools::OpenFaceFXStudio(fxActor, &errMsg))
			{
				ShowError(FText::FromString(errMsg));
				anyError = true;
			}
		}
	}

	if(!anyError)
	{
		ShowInfo(LOCTEXT("Asset_OpenAsset","Opening asset(s) in background process successfully completed."));
	}
}

#undef LOCTEXT_NAMESPACE
