#pragma once

#include "FaceFXAsset.h"
#include "IAssetTypeActions.h"
#include "FaceFXActorFactory.generated.h"

UCLASS(hidecategories=Object)
class UFaceFXActorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//UFactory
	virtual UObject* FactoryCreateNew(UClass* InClass,UObject* InParent,FName InName,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Animation; }
	virtual FName GetNewAssetThumbnailOverride() const override
	{
		static const FName Thumbnail(TEXT("FaceFXStyle.AssetFXActor"));
		return Thumbnail;
	}
	//~UFactory

	/**
	* Creates a new asset of the given class. This class must be a subclass of UFaceFXAsset
	* @param Class The class to instantiate
	* @param InParent The owning parent object
	* @param Name The name of the new object
	* @param Flags The flags used during instantiation
	* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @param FaceFXAsset The filepath of the .facefx asset that shall be imported. Keep empty to have a FileOpenDialog showing up
	* @returns The new object or nullptr if not instantiated
	*/
	static UObject* CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback = FCompilationBeforeDeletionDelegate(), FString FaceFXAsset = "");
	
private:

	/** Callback function for before the compilation folder gets deleted */
	UFUNCTION()
	void OnCompilationBeforeDelete(class UObject* Asset, const FString& CompilationFolder, bool LoadResult);
};
