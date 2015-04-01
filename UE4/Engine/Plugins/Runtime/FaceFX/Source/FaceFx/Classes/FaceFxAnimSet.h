#pragma once

#include "FaceFxAsset.h"
#include "FaceFxData.h"
#include "FaceFxAnimSet.generated.h"

/** Asset that contains a set of animations */
UCLASS(hideCategories=Object)
class FACEFX_API UFaceFxAnimSet : public UFaceFxAsset
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR

	friend struct FFaceFxEditorTools;

	//UObject
	virtual void Serialize(FArchive& Ar) override;
	//~UObject

#endif //WITH_EDITOR

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline FFaceFxAnimSetData& GetData()
	{
		checkf(m_platformData.Num(), TEXT("Asset not initialized yet."));

		//always take the first entry.
		//For non-cooked this will always be PC
		//For cooked this will always be the data from the target platform
		return m_platformData[0];
	}

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline const FFaceFxAnimSetData& GetData() const
	{
		checkf(m_platformData.Num(), TEXT("Asset not initialized yet."));

		//always take the first entry.
		//For non-cooked this will always be PC
		//For cooked this will always be the data from the target platform
		return m_platformData[0];
	}

	/**
	* Looks for a specific animation with the given name
	* @param animName The name of the animation to look for
	* @returns The animation data or nullptr if not found
	*/
	inline const FFaceFxAnimData* FindAnimation(const FName& animName) const
	{
		return m_platformData.Num() > 0 ? GetData().m_animations.FindByKey(animName) : nullptr;
	}

	/**
	* Gets the name of the animation group
	* @returns The name
	*/
	inline const FName& GetGroup() const
	{
		return m_group;
	}

	/**
	* Checks if this FaceFX data asset it valid
	* @returns True if valid, else false
	*/
	virtual bool IsValid() const override
	{
		return IsAssetPathSet() && m_platformData.Num() > 0;
	}

	FORCEINLINE bool operator==(const FName& group) const
	{
		return m_group == group;
	}

	/**
	* Gets the details in a human readable string representation
	* @param outDetails The resulting details string
	*/
	virtual void GetDetails(FString& outDetails) const override;

	/**
	* Gets the number of animations which are encapsulated in this asset
	* @return The animation count
	*/
	virtual int32 GetAnimationCount() const override
	{ 
		return GetData().m_animations.Num();
	}

private:

	/** The animation group */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName m_group;

	/** The data inside this data set. Its a list of data per platform. Indices: x86, PS4, XBoxOne */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFxAnimSetData> m_platformData;
};