/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2017 OC3 Entertainment, Inc.
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
#include "Sound/SoundWave.h"
#include "FaceFXAnim.generated.h"

/** Asset that contains a set of animations */
UCLASS(hideCategories=Object)
class FACEFX_API UFaceFXAnim : public UFaceFXAsset
{
	GENERATED_UCLASS_BODY()

public:

	//UObject
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~UObject

	/**
	* Checks if this FaceFX data asset it valid
	* @returns True if valid, else false
	*/
	virtual bool IsValid() const override
	{
		return Super::IsValid() && PlatformData.Num() > 0;
	}

#if WITH_EDITORONLY_DATA

	friend struct FFaceFXEditorTools;

	//UObject
	virtual void Serialize(FArchive& Ar) override;
	//~UObject

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
		return PlatformData.Num() > 0 ? 1 : 0;
	}

	/**
	* Gets the absolute path to the source audio file
	* @param OutResult The absolute path when the function returns true. Unchanged if it returns false
	* @returns True if the path is set and OutResult was updated, else false
	*/
	inline bool GetAbsoluteAudioPath(FString& OutResult) const
	{
		if(!AudioPath.IsEmpty())
		{
			OutResult = GetAssetFolder() / AudioPath;
			return true;
		}
		return false;
	}

	/**
	* Gets the relative audio path that is currently set
	* @returns The relative audio path
	*/
	inline const FString& GetRelativeAudioPath() const
	{
		return AudioPath;
	}

	/**
	* Gets the indicator if the audio path was set
	* @returns True if set, else false
	*/
	inline bool IsAudioPathSet() const
	{
		return !AudioPath.IsEmpty();
	}

	/**
	* Gets the indicator if the Audio asset is valid and points to something not matter if asset exist or not
	* @returns True if valid and points to something, else false
	*/
	inline bool IsAudioAssetSet() const
	{
		return Audio.GetUniqueID().IsValid();
	}

	/**
	* Gets the platform data for the given target platform
	* @returns The data entry or nullptr if not found
	*/
	inline FFaceFXAnimData* GetPlatformData(EFaceFXTargetPlatform::Type Platform = EFaceFXTargetPlatform::PC)
	{
		return PlatformData.FindByKey(EFaceFXTargetPlatform::PC);
	}

	/**
	* Gets the platform data for the given target platform or creates a new entry if missing
	* @returns The data entry or nullptr if not found
	*/
	inline FFaceFXAnimData& GetOrCreatePlatformData(EFaceFXTargetPlatform::Type Platform = EFaceFXTargetPlatform::PC)
	{
		if(FFaceFXAnimData* ExistingEntry = PlatformData.FindByKey(Platform))
		{
			return *ExistingEntry;
		}
		return PlatformData[PlatformData.Add(FFaceFXAnimData(Platform))];
	}

	/**
	* Resets the asset
	* @param ResetAudio Indicator if the audio shall be resetted as well
	*/
	inline void Reset(bool ResetAudio = false)
	{
		PlatformData.Empty();

		if(ResetAudio)
		{
			AudioPath = TEXT("");
			Audio.Reset();
		}
	}

#endif //WITH_EDITORONLY_DATA

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline FFaceFXAnimData& GetData()
	{
#if WITH_EDITORONLY_DATA
		//non-cooked build - this will always be PC
		FFaceFXAnimData* DataForPC = PlatformData.FindByKey(EFaceFXTargetPlatform::PC);
		checkf(DataForPC, TEXT("Asset not initialized for PC."));
		return *DataForPC;
#else
		//cooked build - this will always be the data from the target platform during cooking
		checkf(PlatformData.Num(), TEXT("Asset not initialized yet."));
		return PlatformData[0];
#endif //WITH_EDITORONLY_DATA
	}

	/**
	* Gets the FaceFX data for the current target platform
	* @returns The data for the current target platform
	*/
	inline const FFaceFXAnimData& GetData() const
	{
#if WITH_EDITORONLY_DATA
		//non-cooked build - this will always be PC
		const FFaceFXAnimData* DataForPC = PlatformData.FindByKey(EFaceFXTargetPlatform::PC);
		checkf(DataForPC, TEXT("Asset not initialized for PC."));
		return *DataForPC;
#else
		//cooked build - this will always be the data from the target platform during cooking
		checkf(PlatformData.Num(), TEXT("Asset not initialized yet."));
		return PlatformData[0];
#endif //WITH_EDITORONLY_DATA
	}

	/**
	* Gets the name of the animation group
	* @returns The name
	*/
	inline const FName& GetGroup() const
	{
		return Id.Group;
	}

	/**
	* Gets the name of the animation
	* @returns The name
	*/
	inline const FName& GetName() const
	{
		return Id.Name;
	}

	/**
	* Gets the assigned audio asset
	* @returns The audio asset
	*/
	inline const TAssetPtr<USoundWave>& GetAudio() const
	{
		return Audio;
	}

	/**
	* Gets the indicator if the group and animation name are set
	* @returns True if set, else false
	*/
	inline bool IsIdSet() const
	{
		return !Id.Group.IsNone() && !Id.Name.IsNone();
	}

	/**
	* Gets the animation id
	* @returns The animation id
	*/
	inline const FFaceFXAnimId& GetId() const
	{
		return Id;
	}

	/**
	* Gets the animation id
	* @returns The animation id
	*/
	inline FFaceFXAnimId& GetId()
	{
		return Id;
	}

	FORCEINLINE bool operator==(const FFaceFXAnimId& InId) const
	{
		return Id == InId;
	}

private:

	/** The animation data. Its a list of data per platform. Will be cooked out to only the target platform. */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFXAnimData> PlatformData;

	/** The animation Id */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FFaceFXAnimId Id;

	/** The linked audio asset */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TAssetPtr<USoundWave> Audio;

#if WITH_EDITORONLY_DATA

	/** The relative file path to the audio source file */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FString AudioPath;

#endif //WITH_EDITORONLY_DATA
};

/** A set of id's which identify a single animation/component combination */
USTRUCT()
struct FFaceFXAnimComponentSet
{
	GENERATED_BODY()

	/** ID of the linked skel mesh component */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	FFaceFXSkelMeshComponentId SkelMeshComponentId;

	/** The animation to play. Only usable when FACEFX_USEANIMATIONLINKAGE is enabled (see FaceFXConfig.h) */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	FFaceFXAnimId AnimationId;

	/** The animation to play */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	TAssetPtr<UFaceFXAnim> Animation;

	inline void Reset()
	{
		Animation.Reset();
		SkelMeshComponentId.Reset();
		AnimationId.Reset();
	}
};
