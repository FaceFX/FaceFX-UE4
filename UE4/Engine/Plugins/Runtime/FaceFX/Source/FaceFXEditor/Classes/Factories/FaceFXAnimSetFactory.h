#pragma once

#include "IAssetTypeActions.h"
#include "FaceFXAnimSetFactory.generated.h"

UCLASS(hidecategories=Object)
class UFaceFXAnimSetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Animation; }
	virtual FName GetNewAssetThumbnailOverride() const override
	{
		static const FName Thumbnail(TEXT("FaceFXStyle.AssetFXAnimSet"));
		return Thumbnail;
	}

	/** Indicator if we only want to create a new asset without actually loading the content */
	uint8 bOnlyCreate : 1;
};
