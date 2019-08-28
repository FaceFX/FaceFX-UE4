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

#include "Animation/AnimNode_BlendFaceFXAnimation.h"
#include "FaceFX.h"
#include "Animation/FaceFXComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"

DECLARE_CYCLE_STAT(TEXT("Blend FaceFX Animation"), STAT_FaceFXBlend, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Blend FaceFX Animation - Load"), STAT_FaceFXBlendLoad, STATGROUP_FACEFX);

FAnimNode_BlendFaceFXAnimation::FAnimNode_BlendFaceFXAnimation() :
	Alpha(1.F),
	bSkipBoneMappingWithoutNS(false),
	LODThreshold(INDEX_NONE),
	bFaceFXCharacterLoadingCompleted(false)
{
#if !UE_BUILD_SHIPPING
	bIsDebugLocalSpaceBlendShown = false;
#endif
}

void FAnimNode_BlendFaceFXAnimation::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	ComponentPose.Initialize(Context);

	//fix to size of 1 as we reuse this container when apply a single bone transforms.
	//We use this one single container to prevent creation/add/empty of temp containers during runtime per tick and bone.
	//We always directly access [0] assuming an entry was added in here
	if(TargetBlendTransform.Num() == 0)
	{
		TargetBlendTransform.AddZeroed(1);
	}

	LoadFaceFXData(Context.AnimInstanceProxy);
}

void FAnimNode_BlendFaceFXAnimation::CacheBones_AnyThread(const FAnimationCacheBonesContext & Context)
{
	ComponentPose.CacheBones(Context);
}

void FAnimNode_BlendFaceFXAnimation::LoadFaceFXData(FAnimInstanceProxy* AnimInstanceProxy)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXBlendLoad);

	BoneIndices.Empty();

	if(!AnimInstanceProxy)
	{
		//wait until we have a proper anim instance
		bFaceFXCharacterLoadingCompleted = false;
		return;
	}

	if(USkeletalMeshComponent* Component = AnimInstanceProxy->GetSkelMeshComponent())
	{
		AActor* Owner = Component->GetOwner();

		if(!Owner)
		{
			//wait until we have a proper owner
			bFaceFXCharacterLoadingCompleted = false;
			return;
		}

		bFaceFXCharacterLoadingCompleted = true;

		//generate the bone mapping indices out of the bone names
		if(UFaceFXComponent* FaceFXComp = Owner->FindComponentByClass<UFaceFXComponent>())
		{
			if(UFaceFXCharacter* FaceFXChar = FaceFXComp->GetCharacter(Component))
			{
				BlendMode = FaceFXChar->GetBlendMode();

				const TArray<FName>& BoneNames = FaceFXChar->GetBoneNames();
				const TArray<FTransform>& BoneRefPoses = Component->SkeletalMesh->RefSkeleton.GetRefBonePose();

				for(const FName& BoneName : BoneNames)
				{
					//find index where the transforms of this
					const int32 BoneTransformIdx = FaceFXChar->GetBoneNameTransformIndex(BoneName);
					if(BoneTransformIdx != INDEX_NONE)
					{
						//find skeleton bone index
						int32 BoneIdx = Component->GetBoneIndex(BoneName);

						if (BoneIdx == INDEX_NONE && !bSkipBoneMappingWithoutNS)
						{
							//strip any existing namespace from the bone name and try matching against it
							FString BoneNameWithOutNS = BoneName.ToString();
							int32 LastNSLocation;
							if (BoneNameWithOutNS.FindLastChar(':', LastNSLocation) && BoneNameWithOutNS.Len() > LastNSLocation)
							{
								BoneNameWithOutNS = BoneNameWithOutNS.RightChop(LastNSLocation + 1);
								BoneIdx = Component->GetBoneIndex(*BoneNameWithOutNS);
							}
						}

						if(BoneIdx != INDEX_NONE)
						{
							const FTransform& BoneRefPose = BoneRefPoses[BoneIdx];

							BoneIndices.Add(FBlendFacialAnimationEntry(BoneIdx, BoneTransformIdx, BoneRefPose));
						}
						else
						{
							UE_LOG(LogFaceFX, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone within skeletal mesh. Bone: %s. SkelMesh: %s. Actor: %s"),
								*BoneName.GetPlainNameString(), *GetNameSafe(Component->SkeletalMesh), *GetNameSafe(Component->GetOwner()));
						}
					}
					else
					{
						UE_LOG(LogFaceFX, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone transformation index. Bone: %i. Actor: %s"),
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
				bFaceFXCharacterLoadingCompleted = !FaceFXComp->IsLoadingCharacterAsync() && FaceFXComp->IsRegistered();
			}
		}
		else
		{
			bFaceFXCharacterLoadingCompleted = false;
		}
	}
}

void FAnimNode_BlendFaceFXAnimation::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	ComponentPose.Update(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
}

void FAnimNode_BlendFaceFXAnimation::Evaluate_AnyThread(FPoseContext& Output)
{
	//convert on the fly to component space and back - This should not happen as the node uses CS.
	//Yet it happened on invalid VIMs. When this happens the blend node needs to be relinked and the VIM needs to be recompiled and saved
	FComponentSpacePoseContext InputCSPose(Output.AnimInstanceProxy);
	EvaluateComponentSpace_AnyThread(InputCSPose);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(InputCSPose.Pose, Output.Pose);

#if !UE_BUILD_SHIPPING
	if(!bIsDebugLocalSpaceBlendShown)
	{
		//show warning only once per node to prevent excessive log spam
		UE_LOG(LogFaceFX, Warning, TEXT("FAnimNode_BlendFacialAnimation::Evaluate. The blend node is using local space input. Please check, relink the blend node and resave VIM. %s. Also contact a FaceFX programmer."), *GetNameSafe(Output.AnimInstanceProxy->GetAnimInstanceObject()));
		bIsDebugLocalSpaceBlendShown = true;
	}
#endif
}

bool FaceFXContainsNaN(const TArray<FBoneTransform>& BoneTransforms)
{
	for (int32 i = 0; i < BoneTransforms.Num(); ++i)
	{
		if (BoneTransforms[i].Transform.ContainsNaN())
		{
			return true;
		}
	}

	return false;
}

void FAnimNode_BlendFaceFXAnimation::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXBlend);

	ComponentPose.EvaluateComponentSpace(Output);

	if(!Output.AnimInstanceProxy)
	{
		return;
	}

	if(!IsLODEnabled(Output.AnimInstanceProxy))
	{
		return;
	}

	if(!bFaceFXCharacterLoadingCompleted)
	{
		//character not done loading yet -> try to retrieve again
		LoadFaceFXData(Output.AnimInstanceProxy);
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

	if(USkeletalMeshComponent* Component = Output.AnimInstanceProxy->GetSkelMeshComponent())
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
					FCompactPoseBoneIndex CompactPoseBoneIndex = Output.Pose.GetPose().GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneIdx));

					// Skip this bone if it doesn't exist at the current LOD level.
					if(CompactPoseBoneIndex.GetInt() == INDEX_NONE)
					{
						continue;
					}

					//fill target transform
					TargetBlendTransform[0].BoneIndex = CompactPoseBoneIndex;

					//convenience alias
					FTransform& BoneTM = TargetBlendTransform[0].Transform;

					//apply transformations in bone space
					if(BlendMode == EFaceFXBlendMode::Replace)
					{
						BoneTM = FaceFXBoneTM;
					}
					else
					{
						//additive mode
						BoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneIndex);

						//convert to Bone Space
						FAnimationRuntime::ConvertCSTransformToBoneSpace(FTransform::Identity, Output.Pose, BoneTM, CompactPoseBoneIndex, EBoneControlSpace::BCS_ParentBoneSpace);

						BoneTM.SetScale3D(BoneTM.GetScale3D() + FaceFXBoneTM.GetScale3D());
						BoneTM.SetRotation(FaceFXBoneTM.GetRotation() * BoneTM.GetRotation());
						BoneTM.AddToTranslation(FaceFXBoneTM.GetTranslation());
					}

					//convert back to Component Space
					FAnimationRuntime::ConvertBoneSpaceTransformToCS(FTransform::Identity, Output.Pose, BoneTM, CompactPoseBoneIndex, EBoneControlSpace::BCS_ParentBoneSpace);

					//sanity check
					checkSlow(!FaceFXContainsNaN(TargetBlendTransform));

					//apply to pose after each bone transform update in order to have proper parent transforms when update childs
					Output.Pose.LocalBlendCSBoneTransforms(TargetBlendTransform, BlendWeight);
				}
			}
		}
	}
}
