#pragma once

#include "FaceFxAsset.h"
#include "IAssetTypeActions.h"
#include "FaceFxActorFactory.generated.h"

UCLASS(hidecategories=Object)
class UFaceFxActorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//UFactory
	virtual UObject* FactoryCreateNew(UClass* InClass,UObject* InParent,FName InName,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Animation; }
	virtual FName GetNewAssetThumbnailOverride() const override
	{
		static const FName s_thumbnail(TEXT("FaceFxStyle.AssetFxActor"));
		return s_thumbnail;
	}
	//~UFactory

	/**
	* Creates a new asset of the given class. This class must be a subclass of UFaceFxAsset
	* @param Class The class to instantiate
	* @param InParent The owning parent object
	* @param Name The name of the new object
	* @param Flags The flags used during instantiation
	* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @param FaceFxAsset The filepath of the .facefx asset that shall be imported. Keep empty to have a FileOpenDialog showing up
	* @returns The new object or nullptr if not instantiated
	*/
	static UObject* CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback = FCompilationBeforeDeletionDelegate(), FString FaceFxAsset = "");
	
private:

	/** Callback function for before the compilation folder gets deleted */
	UFUNCTION()
	void OnCompilationBeforeDelete(class UObject* asset, const FString& compilationFolder, bool loadResult);
};
