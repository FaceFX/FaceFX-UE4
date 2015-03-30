#pragma once

#include "IAssetTypeActions.h"
#include "FaceFxAnimSetFactory.generated.h"

UCLASS(hidecategories=Object)
class UFaceFxAnimSetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;

#if WITH_FACEFX
	virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::FaceFX; }
#endif

	/** Indicator if we only want to create a new asset without actually loading the content */
	uint8 m_onlyCreate : 1;
};
