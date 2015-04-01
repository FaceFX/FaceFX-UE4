#pragma once

#include "Animation/AnimNodeBase.h"
#include "Animation/BoneControllers/AnimNode_ModifyBone.h"

#include "AnimNode_BlendFacialAnimation.generated.h"

/** Anim graph node that blends in facial animation bone transforms into the pose */
USTRUCT()
struct FACEFX_API FAnimNode_BlendFacialAnimation : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	FAnimNode_BlendFacialAnimation();

	// Input link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FComponentSpacePoseLink ComponentPose;

	/** The strength of blending in the facial animation. Will be capped to .0F to 1.F */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BlendMode, meta=(PinShownByDefault))
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
		FBlendFacialAnimationEntry(int32 boneIdx, int32 transformIdx) : m_boneIdx(boneIdx), m_transformIdx(transformIdx) {}

		int32 m_boneIdx;
		int32 m_transformIdx;
	};

	/** The bone indices where to copy the transforms into. Based on the bone names coming from the facefx character instance */
	TArray<FBlendFacialAnimationEntry> m_boneIndices;

	/**
	* Try to load the FaceFX character data
	* @param animInstance The anim graph instance to use
	*/
	void LoadFaceFx(UAnimInstance* animInstance);

	/** The container where we put the current transforms into that are about to get blended. We always only use the very first entry */
	UPROPERTY()
	TArray<FBoneTransform> m_targetBlendTransform;

	/** Indicator if the async loading process of the FaceFX character has completed. */
	uint8 m_faceFxCharacterLoadingCompleted : 1;

	/** TODO: Remove again once engine issue have been localized */
	uint8 m_isDebugLocalSpaceBlendShown : 1;
};