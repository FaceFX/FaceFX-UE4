#pragma once

#include "AssetTypeActions_FaceFXBase.h"

class FAssetTypeActions_FaceFXAnimSet : public FAssetTypeActions_FaceFXBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;

private:

	/** Handler for when Link is selected */
	void ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when Unlink is selected */
	void ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Callback for when the assets to link with have been chosen for a set of objects */
	static void OnAssetLinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects);

	/** Callback for when the assets to unlink with have been chosen for a set of objects */
	static void OnAssetUnlinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects);
};
