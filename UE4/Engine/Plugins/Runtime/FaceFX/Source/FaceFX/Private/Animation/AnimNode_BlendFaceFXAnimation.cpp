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

#include "FaceFX.h"
#include "Animation/AnimNode_BlendFaceFXAnimation.h"
#include "Animation/FaceFXComponent.h"

DECLARE_CYCLE_STAT(TEXT("Blend FaceFX Animation"), STAT_FaceFXBlend, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Blend FaceFX Animation - Load"), STAT_FaceFXBlendLoad, STATGROUP_FACEFX);

FAnimNode_BlendFaceFXAnimation::FAnimNode_BlendFaceFXAnimation() :
	Alpha(1.F),
	TranslationMode(BMM_Replace),
	RotationMode(BMM_Replace),
	ScaleMode(BMM_Replace),
	bFaceFXCharacterLoadingCompleted(false)
{
#if !UE_BUILD_SHIPPING
	bIsDebugLocalSpaceBlendShown = false;
#endif
}

void FAnimNode_BlendFaceFXAnimation::Initialize(const FAnimationInitializeContext& Context)
{
	ComponentPose.Initialize(Context);

	//fix to size of 1 as we reuse this container when apply a single bone transforms. 
	//We use this one single container to prevent creation/add/empty of temp containers during runtime per tick and bone. 
	//We always directly access [0] assuming an entry was added in here
	if(TargetBlendTransform.Num() == 0)
	{
		TargetBlendTransform.AddZeroed(1);
	}
}

void FAnimNode_BlendFaceFXAnimation::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	ComponentPose.CacheBones(Context);

	LoadFaceFXData(Context.AnimInstance);
}

void FAnimNode_BlendFaceFXAnimation::LoadFaceFXData(UAnimInstance* AnimInstance)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FaceFXBlendLoad);

	BoneIndices.Empty();
	bFaceFXCharacterLoadingCompleted = true;

	if(USkeletalMeshComponent* Component = AnimInstance ? AnimInstance->GetSkelMeshComponent() : nullptr)
	{
		AActor* Owner = Component->GetOwner();

		if(UFaceFXComponent* FaceFXComp = Owner ? Owner->FindComponentByClass<UFaceFXComponent>() : nullptr)
		{
			if(UFaceFXCharacter* FaceFXChar = FaceFXComp->GetCharacter(Component))
			{
				const TArray<FName>& BoneNames = FaceFXChar->GetBoneNames();

				for(const FName& BoneName : BoneNames)
				{
					//find index where the transforms of this 
					const int32 BoneTransformIdx = FaceFXChar->GetBoneNameTransformIndex(BoneName);
					if(BoneTransformIdx != INDEX_NONE)
					{
						//find skeleton bone index
						const int32 BoneIdx = Component->GetBoneIndex(BoneName);
						if(BoneIdx != INDEX_NONE)
						{
							BoneIndices.Add(FBlendFacialAnimationEntry(BoneIdx, BoneTransformIdx));
						}
						else
						{
							UE_LOG(LogAnimation, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone within skeletal mesh. Bone: %s. SkelMesh: %s. Actor: %s"), 
								*BoneName.GetPlainNameString(), *GetNameSafe(Component->SkeletalMesh), *GetNameSafe(Component->GetOwner()));
						}
					}
					else
					{
						UE_LOG(LogAnimation, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone transformation index. Bone: %i. Actor: %s"), 
							*BoneName.GetPlainNameString(), *GetNameSafe(Component->GetOwner()));
					}
				}

				//sort in parents before children order
				struct BlendFacialAnimationSort
				{
					FORCEINLINE bool operator()(const FBlendFacialAnimationEntry& A, const FBlendFacialAnimationEntry& B) const
					{
						return A.BoneIdx < B.BoneIdx;
					}
				};
				BoneIndices.Sort(BlendFacialAnimationSort());
			}
			else
			{
				//no FaceFX character exist yet -> check if we're currently loading one async
				bFaceFXCharacterLoadingCompleted = !FaceFXComp->IsLoadingCharacterAsync();
			}
		}
	}
}

void FAnimNode_BlendFaceFXAnimation::Update(const FAnimationUpdateContext& Context)
{
	ComponentPose.Update(Context);
	EvaluateGraphExposedInputs.Execute(Context);
}

void FAnimNode_BlendFaceFXAnimation::Evaluate(FPoseContext& Output)
{
	//convert on the fly to component space and back - This should not happen as the node uses CS. 
	//Yet it happened on invalid VIMs. When this happens the blend node needs to be relinked and the VIM needs to be recompiled and saved
	FComponentSpacePoseContext InputCSPose(Output.AnimInstance);
	EvaluateComponentSpace(InputCSPose);
	InputCSPose.Pose.ConvertToLocalPoses(Output.Pose);

#if !UE_BUILD_SHIPPING
	if(!bIsDebugLocalSpaceBlendShown)
	{
		//show warning only once per node to prevent excessive log spam
		UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_BlendFacialAnimation::Evaluate. The blend node is using local space input. Please check, relink the blend node and resave VIM. %s. Also contact a FaceFX programmer."), *GetNameSafe(Output.AnimInstance));
		bIsDebugLocalSpaceBlendShown = true;
	}
#endif
}

void FAnimNode_BlendFaceFXAnimation::EvaluateComponentSpace(FComponentSpacePoseContext& Output)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FaceFXBlend);

	ComponentPose.EvaluateComponentSpace(Output);

	if(!Output.AnimInstance)
	{
		return;
	}

	if(!bFaceFXCharacterLoadingCompleted)
	{
		//character not done loading yet -> try to retrieve again
		LoadFaceFXData(Output.AnimInstance);
	}

	if(BoneIndices.Num() <= 0)
	{
		//nothing to blend in
		return;
	}

	const float BlendWeight = FMath::Clamp(Alpha, 0.f, 1.f);
	if(BlendWeight <= 0.F)
	{
		//nothing to blend in
		return;
	}

	if(USkeletalMeshComponent* Component = Output.AnimInstance->GetSkelMeshComponent())
	{
		const AActor* Owner = Component->GetOwner();

		if(UFaceFXComponent* FaceFXComp = Owner ? Owner->FindComponentByClass<UFaceFXComponent>() : nullptr)
		{
			if(UFaceFXCharacter* FaceFXChar = FaceFXComp->GetCharacter(Component))
			{
				const TArray<FTransform>& FaceFXBoneTransforms = FaceFXChar->GetBoneTransforms();

				for(const FBlendFacialAnimationEntry& Entry : BoneIndices)
				{
					const FTransform& FaceFXBoneTM = FaceFXBoneTransforms[Entry.TransformIdx];
					const int32 BoneIdx = Entry.BoneIdx;

					//fill target transform
					TargetBlendTransform[0].Transform = Output.Pose.GetComponentSpaceTransform(BoneIdx);
					TargetBlendTransform[0].BoneIndex = BoneIdx;

					//convenience alias
					FTransform& BoneTM = TargetBlendTransform[0].Transform;

					//convert to Bone Space
					FAnimationRuntime::ConvertCSTransformToBoneSpace(Component, Output.Pose, BoneTM, BoneIdx, BCS_ParentBoneSpace);

					//apply transformations in bone space
					if(TranslationMode == BMM_Replace && RotationMode == BMM_Replace && ScaleMode == BMM_Replace)
					{
						//full replace
						BoneTM = FaceFXBoneTM;
					}
					else
					{
						//Scale first
						switch(ScaleMode)
						{
						case BMM_Additive: BoneTM.SetScale3D(BoneTM.GetScale3D() * FaceFXBoneTM.GetScale3D()); break;
						case BMM_Replace: BoneTM.SetScale3D(FaceFXBoneTM.GetScale3D()); break;
						}

						switch(RotationMode)
						{
						case BMM_Additive: BoneTM.SetRotation(FaceFXBoneTM.GetRotation() * BoneTM.GetRotation()); break;
						case BMM_Replace: BoneTM.SetRotation(FaceFXBoneTM.GetRotation()); break;
						}

						//blend by mode
						switch(TranslationMode)
						{
						case BMM_Additive: BoneTM.AddToTranslation(FaceFXBoneTM.GetTranslation()); break;
						case BMM_Replace: BoneTM.SetTranslation(FaceFXBoneTM.GetTranslation()); break;
						}
					}

					//convert back to Component Space
					FAnimationRuntime::ConvertBoneSpaceTransformToCS(Component, Output.Pose, BoneTM, Entry.BoneIdx, BCS_ParentBoneSpace);

					//sanity check
					//checkSlow(!ContainsNaN(resultTransforms));

					//apply to pose after each bone transform update in order to have proper parent transforms when update childs
					Output.Pose.LocalBlendCSBoneTransforms(TargetBlendTransform, BlendWeight);
				}
			}
		}
	}
}
