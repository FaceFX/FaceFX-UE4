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

#include "Animation/AnimNodeBase.h"
#include "Animation/BoneControllers/AnimNode_ModifyBone.h"

#include "AnimNode_BlendFaceFXAnimation.generated.h"

/** Anim graph node that blends in facial animation bone transforms into the pose */
USTRUCT()
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

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BlendMode)
	TEnumAsByte<EBoneModificationMode> TranslationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BlendMode)
	TEnumAsByte<EBoneModificationMode> RotationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BlendMode)
	TEnumAsByte<EBoneModificationMode> ScaleMode;

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) override;
	virtual void Update(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;
	virtual void EvaluateComponentSpace(FComponentSpacePoseContext& Output) override;
	// End of FAnimNode_Base interface

private:

	/** struct that holds a transform / boneidx mapping */
	struct FBlendFacialAnimationEntry
	{
		FBlendFacialAnimationEntry(int32 InBoneIdx, int32 InTransformIdx) : BoneIdx(InBoneIdx), TransformIdx(InTransformIdx) {}

		int32 BoneIdx;
		int32 TransformIdx;
	};

	/** The bone indices where to copy the transforms into. Based on the bone names coming from the facefx character instance */
	TArray<FBlendFacialAnimationEntry> BoneIndices;

	/**
	* Try to load the FaceFX character data
	* @param AnimInstance The anim graph instance to use
	*/
	void LoadFaceFXData(UAnimInstance* AnimInstance);

	/** The container where we put the current transforms into that are about to get blended. We always only use the very first entry */
	UPROPERTY()
	TArray<FBoneTransform> TargetBlendTransform;

	/** Indicator if the async loading process of the FaceFX character has completed. */
	uint8 bFaceFXCharacterLoadingCompleted : 1;

#if !UE_BUILD_SHIPPING
	/** TODO: Remove again once engine issue have been localized */
	uint8 bIsDebugLocalSpaceBlendShown : 1;
#endif
};
