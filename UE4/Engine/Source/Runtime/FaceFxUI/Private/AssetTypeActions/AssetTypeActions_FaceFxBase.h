#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_FaceFxBase : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FColor GetTypeColor() const override { return FColor(137,191,53); }
	virtual bool CanFilter() override { return true; }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::FaceFX; }

protected:

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when SetSource is selected */
	void ExecuteSetSource(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when ShowDetails is selected */
	void ExecuteShowDetails(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can recompile assets */
	bool CanExecuteReimport(const TArray<UObject*> Objects) const;

	/** Determine if we can set a new source */
	bool CanExecuteSetSource(const TArray<UObject*> Objects) const;

	/** Determine if we can show details */
	bool CanExecuteShowDetails(const TArray<UObject*> Objects) const;

#if PLATFORM_WINDOWS

	/** Handler for when OpenFolder is selected */
	void ExecuteOpenFolder(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can open the source asset folder */
	bool CanExecuteOpenFolder(const TArray<UObject*> Objects) const;

#endif

	/** 
	* Shows a slate error message
	* @param msg The error message to show
	*/
	static void ShowError(const FText& msg);

	/** 
	* Shows a slate info message
	* @param msg The info message to show
	*/
	static void ShowInfo(const FText& msg);

private:

	/** Callback function for before the compilation folder gets deleted */
	void OnReimportBeforeDelete(class UObject* asset, const FString& compilationFolder, bool loadResult);
};
