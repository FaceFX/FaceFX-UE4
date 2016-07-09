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

#include "FaceFXData.h"
#include "MovieSceneSection.h"

#include "FaceFXAnimationSection.generated.h"

class UFaceFXAnim;

/** FaceFX animation section for the movie tracks */
UCLASS(MinimalAPI)
class UFaceFXAnimationSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()

public:
	
	inline void SetData(const FFaceFXAnimComponentSet& AnimCompSet)
	{
		Animation = AnimCompSet.Animation;
		AnimationId = AnimCompSet.AnimationId;
		SkelMeshComponentId = AnimCompSet.SkelMeshComponentId;
	}

	/** Sets the skel mesh component id */
	inline void SetComponent(const FFaceFXSkelMeshComponentId& Component)
	{ 
		SkelMeshComponentId = Component;
	}

	/** Gets the assigned skel mesh component id */
	inline const FFaceFXSkelMeshComponentId& GetComponent() const
	{
		return SkelMeshComponentId;
	}

	/** Sets the animation id */
	inline void SetAnimationId(const FFaceFXAnimId& AnimId)
	{
		AnimationId = AnimId;
	}

	/** Gets the assigned animation id */
	inline const FFaceFXAnimId& GetAnimationId() const
	{
		return AnimationId;
	}

	/** Sets the animation asset */
	inline void SetAnimation(const TAssetPtr<UFaceFXAnim>& Anim)
	{
		Animation = Anim;
	}

	/** Gets the assigned animation asset */
	inline const TAssetPtr<UFaceFXAnim>& GetAnimationAsset() const
	{
		return Animation;
	}
	
	/**
	* Loads the animation represented by this section
	* @param Owner The owner of the potentially loaded animation data
	* @returns The loaded animation, else nullptr if not found or unset
	*/
	UFaceFXAnim* GetAnimation(UObject* Owner = nullptr) const;

	/**
	* Gets the duration of the assigned animation
	* @param Actor The contextual actor to fetch the animation id from if needed
	* @returns The animation duration
	*/
	float FACEFX_API GetAnimationDuration(const AActor* Actor = nullptr) const;

#if WITH_EDITOR
	/** Gets the indicator if the animation duration is cached already */
	inline bool FACEFX_API IsAnimationDurationLoaded() const
	{
		return bIsAnimationDurationLoaded;
	}

	FText FACEFX_API GetTitle() const;

	//UObject
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~UObject
#endif

	/** Gets the start offset into the animation clip */
	inline float GetStartOffset() const 
	{ 
		return StartOffset; 
	}

	/** Sets the start offset into the animation clip */
	inline void SetStartOffset(float InStartOffset) 
	{ 
		StartOffset = InStartOffset; 
	}

	/** Gets the end offset into the animation clip */
	inline float GetEndOffset() const 
	{ 
		return EndOffset; 
	}

	/** Sets the end offset into the animation clip */
	inline void SetEndOffset(float InEndOffset) 
	{ 
		EndOffset = InEndOffset; 
	}

	//UMovieSceneSection
	virtual UMovieSceneSection* SplitSection(float SplitTime) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& OutKeyHandles, TRange<float> TimeRange) const override { }
	virtual void GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const override;
	virtual TOptional<float> GetKeyTime(FKeyHandle KeyHandle) const override { return TOptional<float>(); }
	virtual void SetKeyTime(FKeyHandle KeyHandle, float Time) override { }
	//~UMovieSceneSection

private:

	/** Gets the owning track */
	class UFaceFXAnimationTrack* GetTrack() const;

	/** Gets the actor associated with the owning track */
	AActor* GetActor() const;

	/** The if of the skel mesh component where this key is working on */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	FFaceFXSkelMeshComponentId SkelMeshComponentId;

	/** The animation to play. Only usable when FACEFX_USEANIMATIONLINKAGE is enabled (see FaceFXConfig.h) */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	FFaceFXAnimId AnimationId;

	/** The animation to play */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	TAssetPtr<UFaceFXAnim> Animation;

	/** The offset into the beginning of the animation clip */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	float StartOffset;

	/** The offset into the end of the animation clip */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	float EndOffset;

	/** The time of the key along the track time line */
	UPROPERTY(Transient)
	float Time;

	/** Indicator if the facial animation duration was loaded and cached already */
	UPROPERTY(Transient)
	mutable uint8 bIsAnimationDurationLoaded : 1;

	/** The cached duration of the selected animation */
	UPROPERTY(Transient)
	mutable float AnimationDuration;
};
