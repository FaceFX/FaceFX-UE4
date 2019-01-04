/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2019 OC3 Entertainment, Inc. All rights reserved.
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
UCLASS(BlueprintType, hideCategories=Object)
class FACEFX_API UFaceFXAnim : public UFaceFXAsset
{
	GENERATED_UCLASS_BODY()

public:

	//UObject
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~UObject

	virtual bool IsValid() const override
	{
		return Super::IsValid() && AnimData.IsValid();
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

	virtual int32 GetAnimationCount() const override
	{
		return AnimData.IsValid() ? 1 : 0;
	}

	bool GetAbsoluteAudioPath(FString& OutResult) const;

	inline const FString& GetRelativeAudioPath() const
	{
		return AudioPath;
	}

	inline bool IsAudioPathSet() const
	{
		return !AudioPath.IsEmpty();
	}

	inline bool IsAudioAssetSet() const
	{
		return Audio.GetUniqueID().IsValid();
	}

	inline void Reset(bool ResetAudio = false)
	{
		AnimData.Reset();

		if(ResetAudio)
		{
			AudioPath = TEXT("");
			Audio.Reset();
		}
	}

#endif //WITH_EDITORONLY_DATA

	inline FFaceFXAnimData& GetData()
	{
		return AnimData;
	}

	inline const FFaceFXAnimData& GetData() const
	{
		return AnimData;
	}

	inline const FName& GetGroup() const
	{
		return Id.Group;
	}

	inline const FName& GetName() const
	{
		return Id.Name;
	}

	inline const TSoftObjectPtr<USoundWave>& GetAudio() const
	{
		return Audio;
	}

	inline const TSoftObjectPtr<UObject>& GetAudioAkEvent() const
	{
		return AudioAkEvent;
	}

	inline const TSoftObjectPtr<UObject>& GetAudioAkEventStop() const
	{
		return AudioAkEventStop;
	}

	inline const TSoftObjectPtr<UObject>& GetAudioAkEventPause() const
	{
		return AudioAkEventPause;
	}

	inline const TSoftObjectPtr<UObject>& GetAudioAkEventResume() const
	{
		return AudioAkEventResume;
	}

	inline bool IsIdSet() const
	{
		return !Id.Group.IsNone() && !Id.Name.IsNone();
	}

	inline const FFaceFXAnimId& GetId() const
	{
		return Id;
	}

	inline FFaceFXAnimId& GetId()
	{
		return Id;
	}

	FORCEINLINE bool operator==(const FFaceFXAnimId& InId) const
	{
		return Id == InId;
	}

private:

	UPROPERTY()
	TArray<FFaceFXAnimData> PlatformData_DEPRECATED;

	/** The animation data. */
	UPROPERTY(VisibleInstanceOnly, Category=FaceFX)
	FFaceFXAnimData AnimData;

	/** The animation Id */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FFaceFXAnimId Id;

	/** The linked audio asset */
	UPROPERTY(EditInstanceOnly, Category=Audio)
	TSoftObjectPtr<USoundWave> Audio;

	/** The linked Wwise audio event asset for: Play */
	UPROPERTY(EditInstanceOnly, Category=AkAudio)
	TSoftObjectPtr<UObject> AudioAkEvent;

	/** The linked Wwise audio event asset for: Stop */
	UPROPERTY(EditInstanceOnly, Category = AkAudio)
	TSoftObjectPtr<UObject> AudioAkEventStop;

	/** The linked Wwise audio event asset for: Pause */
	UPROPERTY(EditInstanceOnly, Category = AkAudio)
	TSoftObjectPtr<UObject> AudioAkEventPause;

	/** The linked Wwise audio event asset for: Resume */
	UPROPERTY(EditInstanceOnly, Category = AkAudio)
	TSoftObjectPtr<UObject> AudioAkEventResume;

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
	TSoftObjectPtr<UFaceFXAnim> Animation;

	inline void Reset()
	{
		Animation.Reset();
		SkelMeshComponentId.Reset();
		AnimationId.Reset();
	}
};
