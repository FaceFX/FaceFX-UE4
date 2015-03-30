// FaceFX_BEGIN
#include "EnginePrivate.h"

#include "Animation/AnimNode_BlendFacialAnimation.h"

#if WITH_FACEFX
#include "FaceFx.h"
#endif

FAnimNode_BlendFacialAnimation::FAnimNode_BlendFacialAnimation() :
	Alpha(1.F),
	TranslationMode(BMM_Replace),
	RotationMode(BMM_Replace),
	ScaleMode(BMM_Replace)
#if WITH_FACEFX
	,m_faceFxCharacterLoadingCompleted(false)
#endif
	, m_isDebugLocalSpaceBlendShown(false)
{
}

void FAnimNode_BlendFacialAnimation::Initialize(const FAnimationInitializeContext& Context)
{
	ComponentPose.Initialize(Context);

#if WITH_FACEFX
	//fix to size of 1 as we reuse this container when apply a single bone transforms. 
	//We use this one single container to prevent creation/add/empty of temp containers during runtime per tick and bone. 
	//We always directly access [0] assuming an entry was added in here
	if(m_targetBlendTransform.Num() == 0)
	{
		m_targetBlendTransform.AddZeroed(1);
	}
#endif
}

void FAnimNode_BlendFacialAnimation::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	ComponentPose.CacheBones(Context);

#if WITH_FACEFX
	LoadFaceFx(Context.AnimInstance);
#endif
}

#if WITH_FACEFX
/**
* Try to load the FaceFX character data
* @param animInstance The anim graph instance to use
*/
void FAnimNode_BlendFacialAnimation::LoadFaceFx(UAnimInstance* animInstance)
{
	m_boneIndices.Empty();
	m_faceFxCharacterLoadingCompleted = true;

	if(USkeletalMeshComponent* component = animInstance ? animInstance->GetSkelMeshComponent() : nullptr)
	{
		if(UFaceFxCharacter* fxChar = component->GetFaceFxCharacter())
		{
#if FACEFX_WITHBONEFILTER
			TArray<FName> boneNames;
			fxChar->GetFilteredBoneNames(boneNames);
#else
			const TArray<FName>& boneNames = fxChar->GetBoneNames();
#endif //FACEFX_WITHBONEFILTER

			for(const FName& boneName : boneNames)
			{
				//find index where the transforms of this 
				const int32 boneTransformIdx = fxChar->GetBoneNameTransformIndex(boneName);
				if(boneTransformIdx != INDEX_NONE)
				{
					//find skeleton bone index
					const int32 boneIdx = component->GetBoneIndex(boneName);
					if(boneIdx != INDEX_NONE)
					{
						m_boneIndices.Add(FBlendFacialAnimationEntry(boneIdx, boneTransformIdx));
					}
					else
					{
						UE_LOG(LogAnimation, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone within skeletal mesh. Bone: %s. SkelMesh: %s. Actor: %s"), 
							*boneName.GetPlainNameString(), *GetNameSafe(component->SkeletalMesh), *GetNameSafe(component->GetOwner()));
					}
				}
				else
				{
					UE_LOG(LogAnimation, Warning, TEXT("BlendFacialAnimation: Unable to find FaceFX bone transformation index. Bone: %i. Actor: %s"), 
						*boneName.GetPlainNameString(), *GetNameSafe(component->GetOwner()));
				}
			}

			//sort in parents before children order
			struct YBlendFacialAnimationSort
			{
				FORCEINLINE bool operator()(const FBlendFacialAnimationEntry& a, const FBlendFacialAnimationEntry& b) const
				{
					return a.m_boneIdx < b.m_boneIdx;
				}
			};
			m_boneIndices.Sort(YBlendFacialAnimationSort());
		}
		else
		{
			//no FaceFX character exist yet -> check if we're currently loading one async
			m_faceFxCharacterLoadingCompleted = !component->IsLoadingFaceFxCharacterAsync();
		}
	}
}
#endif

void FAnimNode_BlendFacialAnimation::Update(const FAnimationUpdateContext& Context)
{
	ComponentPose.Update(Context);
	EvaluateGraphExposedInputs.Execute(Context);
}

void FAnimNode_BlendFacialAnimation::Evaluate(FPoseContext& Output)
{
	//convert on the fly to component space and back - This should not happen as the node uses CS. 
	//Yet it happened on invalid VIMs. When this happens the blend node needs to be relinked and the VIM needs to be recompiled and saved
	FComponentSpacePoseContext InputCSPose(Output.AnimInstance);
	EvaluateComponentSpace(InputCSPose);
	InputCSPose.Pose.ConvertToLocalPoses(Output.Pose);
	 
	if(!m_isDebugLocalSpaceBlendShown)
	{
		//show warning only once per node to prevent excessive log spam
		UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_BlendFacialAnimation::Evaluate. The blend node is using local space input. Please check, relink the blend node and resave VIM. %s. Also contact a FaceFX programmer."), *GetNameSafe(Output.AnimInstance));
		m_isDebugLocalSpaceBlendShown = true;
	}
}

void FAnimNode_BlendFacialAnimation::EvaluateComponentSpace(FComponentSpacePoseContext& Output)
{
	ComponentPose.EvaluateComponentSpace(Output);

#if WITH_FACEFX

	if(!Output.AnimInstance)
	{
		return;
	}

	if(!m_faceFxCharacterLoadingCompleted)
	{
		//character not done loading yet -> try to retrieve again
		LoadFaceFx(Output.AnimInstance);
	}

	if(m_boneIndices.Num() <= 0)
	{
		//nothing to blend in
		return;
	}

	const float blendWeight = FMath::Clamp(Alpha, 0.f, 1.f);
	if(blendWeight <= 0.F)
	{
		//nothing to blend in
		return;
	}

	if(USkeletalMeshComponent* component = Output.AnimInstance->GetSkelMeshComponent())
	{
		if(UFaceFxCharacter* fxChar = component->GetFaceFxCharacter())
		{
			const TArray<FTransform>& fxBoneTransforms = fxChar->GetBoneTransforms();

			for(const FBlendFacialAnimationEntry& entry : m_boneIndices)
			{
				const FTransform& faceFxBoneTM = fxBoneTransforms[entry.m_transformIdx];
				const int32 boneIdx = entry.m_boneIdx;

				//fill target transform
				m_targetBlendTransform[0].Transform = Output.Pose.GetComponentSpaceTransform(boneIdx);
				m_targetBlendTransform[0].BoneIndex = boneIdx;

				//convenience alias
				FTransform& boneTM = m_targetBlendTransform[0].Transform;

				//convert to Bone Space
				FAnimationRuntime::ConvertCSTransformToBoneSpace(component, Output.Pose, boneTM, boneIdx, BCS_ParentBoneSpace);

				//apply transformations in bone space
				if(TranslationMode == BMM_Replace && RotationMode == BMM_Replace && ScaleMode == BMM_Replace)
				{
					//full replace
					boneTM = faceFxBoneTM;
				}
				else
				{
					//blend by mode
					switch(TranslationMode)
					{
					case BMM_Additive: boneTM.AddToTranslation(faceFxBoneTM.GetTranslation()); break;
					case BMM_Replace: boneTM.SetTranslation(faceFxBoneTM.GetTranslation()); break;
					}

					switch(RotationMode)
					{
					case BMM_Additive: boneTM.SetRotation(faceFxBoneTM.GetRotation() * boneTM.GetRotation()); break;
					case BMM_Replace: boneTM.SetRotation(faceFxBoneTM.GetRotation()); break;
					}

					switch(ScaleMode)
					{
					case BMM_Additive: boneTM.SetScale3D(boneTM.GetScale3D() * faceFxBoneTM.GetScale3D()); break;
					case BMM_Replace: boneTM.SetScale3D(faceFxBoneTM.GetScale3D()); break;
					}
				}

				//convert back to Component Space
				FAnimationRuntime::ConvertBoneSpaceTransformToCS(component, Output.Pose, boneTM, entry.m_boneIdx, BCS_ParentBoneSpace);

				//sanity check
				//checkSlow(!ContainsNaN(resultTransforms));

				//apply to pose after each bone transform update in order to have proper parent transforms when update childs
				Output.Pose.LocalBlendCSBoneTransforms(m_targetBlendTransform, blendWeight);
			}
		}
	}

#endif
}
// FaceFX_END