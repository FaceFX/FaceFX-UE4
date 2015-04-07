/*******************************************************************************
  The MIT License (MIT)

  Copyright (c) 2015 OC3 Entertainment, Inc.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*******************************************************************************/

#pragma once

#include "FaceFXAsset.h"
#include "FaceFXData.h"
#include "FaceFXAnimSet.generated.h"

/** Asset that contains a set of animations */
UCLASS(hideCategories=Object)
class FACEFX_API UFaceFXAnimSet : public UFaceFXAsset
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR

	friend struct FFaceFXEditorTools;

	//UObject
	virtual void Serialize(FArchive& Ar) override;
	//~UObject

#endif //WITH_EDITOR

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline FFaceFXAnimSetData& GetData()
	{
		checkf(PlatformData.Num(), TEXT("Asset not initialized yet."));

		//always take the first entry.
		//For non-cooked this will always be PC
		//For cooked this will always be the data from the target platform
		return PlatformData[0];
	}

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline const FFaceFXAnimSetData& GetData() const
	{
		checkf(PlatformData.Num(), TEXT("Asset not initialized yet."));

		//always take the first entry.
		//For non-cooked this will always be PC
		//For cooked this will always be the data from the target platform
		return PlatformData[0];
	}

	/**
	* Looks for a specific animation with the given name
	* @param AnimName The name of the animation to look for
	* @returns The animation data or nullptr if not found
	*/
	inline const FFaceFXAnimData* FindAnimation(const FName& AnimName) const
	{
		return PlatformData.Num() > 0 ? GetData().Animations.FindByKey(AnimName) : nullptr;
	}

	/**
	* Gets the name of the animation group
	* @returns The name
	*/
	inline const FName& GetGroup() const
	{
		return Group;
	}

	/**
	* Checks if this FaceFX data asset it valid
	* @returns True if valid, else false
	*/
	virtual bool IsValid() const override
	{
		return IsAssetPathSet() && PlatformData.Num() > 0;
	}

	FORCEINLINE bool operator==(const FName& InGroup) const
	{
		return Group == InGroup;
	}

	/**
	* Gets the details in a human readable string representation
	* @param OutDetails The resulting details string
	*/
	virtual void GetDetails(FString& OutDetails) const override;

	/**
	* Gets the number of animations which are encapsulated in this asset
	* @return The animation count
	*/
	virtual int32 GetAnimationCount() const override
	{ 
		return GetData().Animations.Num();
	}

private:

	/** The animation group */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName Group;

	/** The data inside this data set. Its a list of data per platform. Indices: x86, PS4, XBoxOne */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFXAnimSetData> PlatformData;
};
