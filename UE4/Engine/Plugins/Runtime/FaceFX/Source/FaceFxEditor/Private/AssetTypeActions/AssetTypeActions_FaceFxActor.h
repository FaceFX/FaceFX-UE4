#pragma once

#include "AssetTypeActions_FaceFxBase.h"

class FAssetTypeActions_FaceFxActor : public FAssetTypeActions_FaceFxBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_FaceFX", "FaceFX Actor"); }
	virtual UClass* GetSupportedClass() const override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor ) override;

private:

	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when Link is selected */
	void ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when Unlink is selected */
	void ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can edit assets */
	bool CanExecuteEdit(const TArray<UObject*> Objects) const;

	/** Callback for when the assets to link with have been chosen for a set of objects */
	static void OnAssetLinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects);

	/** Callback for when the assets to unlink with have been chosen for a set of objects */
	static void OnAssetUnlinkChosen(const TArray<FAssetData>& selectedAssets, TArray<TWeakObjectPtr<UObject>> selectedObjects);
};
