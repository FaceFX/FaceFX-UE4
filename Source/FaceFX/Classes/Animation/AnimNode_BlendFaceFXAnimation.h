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

#include "Animation/AnimationAsset.h"	//required by AnimNodeBase.h
#include "Animation/AnimNodeBase.h"
#include "BoneControllers/AnimNode_ModifyBone.h"

#include "AnimNode_BlendFaceFXAnimation.generated.h"

struct FAnimInstanceProxy;
enum class EFaceFXBlendMode : uint8;

/** Anim graph node that blends in facial animation bone transforms into the pose */
USTRUCT(BlueprintType)
struct FACEFX_API FAnimNode_BlendFaceFXAnimation : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	FAnimNode_BlendFaceFXAnimation();

	// Input link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FComponentSpacePoseLink ComponentPose;

	/** The strength of blending in the facial animation. Will be capped to .0F to 1.F */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BlendMode, meta=(PinShownByDefault, UIMin=0.F, UIMax=1.F))
	float Alpha;

	/** Indicator if stripped name space bone mapping shall be skipped during bone matching phase in case a bone name was not found */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=BoneMapping)
	bool bSkipBoneMappingWithoutNS;

	// @todo What about the transitions where the node becomes active / inactive?
	/** The maximum LOD setting under which this node is allowed to run. Defaults to all LOD settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext & Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface

private:

	/** struct that holds a transform / boneidx mapping */
	struct FBlendFacialAnimationEntry
	{
		FBlendFacialAnimationEntry(int32 InBoneIdx, int32 InTransformIdx, const FTransform& InBoneRefPose) : BoneIdx(InBoneIdx), TransformIdx(InTransformIdx)
		{
			BoneRefPoseScale = InBoneRefPose.GetScale3D();
			BoneRefPoseTranslation = InBoneRefPose.GetTranslation();
			BoneRefPoseRotationInv = InBoneRefPose.GetRotation().Inverse();
		}

		int32 BoneIdx;
		int32 TransformIdx;
		FVector BoneRefPoseScale;
		FVector BoneRefPoseTranslation;
		FQuat BoneRefPoseRotationInv;
	};

	/** The bone indices where to copy the transforms into. Based on the bone names coming from the facefx character instance */
	TArray<FBlendFacialAnimationEntry> BoneIndices;

	/**
	* Try to load the FaceFX character data
	* @param AnimInstance The anim graph instance to use
	*/
	void LoadFaceFXData(FAnimInstanceProxy* AnimInstanceProxy);

	/** The container where we put the current transforms into that are about to get blended. We always only use the very first entry */
	TArray<FBoneTransform> TargetBlendTransform;

	/** Indicator if the async loading process of the FaceFX character has completed. */
	uint8 bFaceFXCharacterLoadingCompleted : 1;

	EFaceFXBlendMode BlendMode;

#if !UE_BUILD_SHIPPING
	/** TODO: Remove again once engine issue have been localized */
	uint8 bIsDebugLocalSpaceBlendShown : 1;
#endif
};
