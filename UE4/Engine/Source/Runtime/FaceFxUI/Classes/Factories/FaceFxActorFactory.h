#pragma once

#include "FaceFxAsset.h"
#include "IAssetTypeActions.h"
#include "FaceFxActorFactory.generated.h"

UCLASS(hidecategories=Object)
class UFaceFxActorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;

#if WITH_FACEFX
	/**
	* Creates a new asset of the given class. This class must be a subclass of UFaceFxAsset
	* @param Class The class to instantiate
	* @param InParent The owning parent object
	* @param Name The name of the new object
	* @param Flags The flags used during instantiation
	* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns The new object or nullptr if not instantiated
	*/
	static UObject* CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback = FCompilationBeforeDeletionDelegate());
	
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::FaceFX; }
#endif

private:

	/** Callback function for before the compilation folder gets deleted */
	UFUNCTION()
	void OnCompilationBeforeDelete(class UObject* asset, const FString& compilationFolder, bool loadResult);
};
