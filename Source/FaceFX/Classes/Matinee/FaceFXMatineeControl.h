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

#include "FaceFXData.h"
#include "Matinee/InterpTrackFloatBase.h"
#include "FaceFXAnim.h"
#include "FaceFXMatineeControl.generated.h"

/** Information for one FaceFX key in the track. */
USTRUCT()
struct FFaceFXTrackKey
{
	GENERATED_USTRUCT_BODY()

	FFaceFXTrackKey() : Time(0.F), bLoop(false), bIsAnimationDurationLoaded(false), AnimationDuration(0.F)	{}

	/** The id of the skel mesh component where this key is working on */
	UPROPERTY(EditAnywhere, Category=FaceFX)
	FFaceFXSkelMeshComponentId SkelMeshComponentId;

	/** The animation to play. Only usable when FACEFX_USEANIMATIONLINKAGE is enabled (see FaceFXConfig.h) */
	UPROPERTY(EditAnywhere, Category=FaceFX)
	FFaceFXAnimId AnimationId;

	/** The animation to play */
	UPROPERTY(EditAnywhere, Category=FaceFX)
	TSoftObjectPtr<UFaceFXAnim> Animation;

	/** The time of the key along the track time line */
	UPROPERTY()
	float Time;

	/** Indicator if the facial animation shall loop */
	UPROPERTY(EditAnywhere, Category=FaceFX)
	uint8 bLoop : 1;

	/**
	* Gets the duration of the assigned animation
	* @param Actor The contextual actor to fetch the animation id from if needed
	* @returns The animation duration
	*/
	float GetAnimationDuration(const AActor* Actor = nullptr) const;

	/** Imports data from a given animation set */
	inline void Import(const FFaceFXAnimComponentSet& Data)
	{
		SkelMeshComponentId = Data.SkelMeshComponentId;
		Animation = Data.Animation;
		AnimationId = Data.AnimationId;
	}

private:

	/** Indicator if the facial animation duration was loaded already*/
	UPROPERTY(Transient)
	mutable uint8 bIsAnimationDurationLoaded : 1;

	/** The duration of the selected animation */
	UPROPERTY(Transient)
	mutable float AnimationDuration;
};

UCLASS(MinimalAPI, meta=( DisplayName = "FaceFX Track" ))
class UFaceFXMatineeControl : public UInterpTrack
{
	GENERATED_UCLASS_BODY()

	friend class UFaceFXMatineeControlHelper;

	//UInterpTrack
	// Some overrides from UInterpTrack to allow inheriting from another module. Those functions are not exposed by the module via ENGINE_API
	virtual const FString GetEdHelperClassName() const override
	{
		return TEXT("");
	}

	virtual FColor GetKeyframeColor(int32 KeyIndex) const override
	{
		static const FColor KeyNormalColor(0,0,0);
		return KeyNormalColor;
	}

#if WITH_EDITORONLY_DATA
	virtual class UTexture2D* GetTrackIcon() const override
	{
		return TrackIcon;
	}
#endif //WITH_EDITORONLY_DATA
	//~UInterpTrack

	//UInterpTrack implementations
	virtual int32 GetNumKeyframes() const override;
	virtual float GetTrackEndTime() const override;
	virtual float GetKeyframeTime( int32 KeyIndex ) const override;
	virtual int32 GetKeyframeIndex( float KeyTime ) const override;
	virtual void GetTimeRange( float& StartTime, float& EndTime ) const override;
	virtual int32 SetKeyframeTime( int32 KeyIndex, float NewKeyTime, bool bUpdateOrder = true ) override;
	virtual void RemoveKeyframe( int32 KeyIndex ) override;
	virtual int32 DuplicateKeyframe( int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack = NULL ) override;
	virtual bool GetClosestSnapPosition( float InPosition, TArray<int32>& IgnoreKeys, float& OutPosition ) override;
	virtual int32 AddKeyframe( float Time, UInterpTrackInst* TrackInst, EInterpCurveMode InitInterpMode ) override;
	virtual void PreviewUpdateTrack( float NewPosition, UInterpTrackInst* TrackInst ) override;
	virtual void PreviewStopPlayback(class UInterpTrackInst* TrInst) override;
	virtual void DrawTrack( FCanvas* Canvas, UInterpGroup* Group, const FInterpTrackDrawParams& Params ) override;
	virtual void UpdateTrack( float NewPosition, UInterpTrackInst* TrackInst, bool bJump ) override;

	virtual const FString GetSlateHelperClassName() const override
	{
		return FString( TEXT("FaceFXEditor.FaceFXMatineeControlHelper") );
	}
	//~UInterpTrack implementations

private:

	/**
	* Gets the keyframe data for a given time on the track
	* @param InTime The time we're currently at on the track
	* @param OutResult The resulting track keys as a pair of track index and key
	* @param OutNoTracks The list of skelmesh components that have no tracks (anymore)
	*/
	void GetTrackKeyForTime(float InTime, TArray<TPair<int32, const FFaceFXTrackKey*>>& OutResult, TArray<FFaceFXSkelMeshComponentId>* OutNoTracks = nullptr) const;

	/** The keys in this track control */
	UPROPERTY()
	TArray<FFaceFXTrackKey> Keys;
};
