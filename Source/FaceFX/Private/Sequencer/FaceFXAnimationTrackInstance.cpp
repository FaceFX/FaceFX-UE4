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
#include "Sequencer/FaceFXAnimationTrackInstance.h"

#include "Sequencer/FaceFXAnimationTrack.h"
#include "Sequencer/FaceFXAnimationSection.h"
#include "Animation/FaceFXComponent.h"

#include "IMovieScenePlayer.h"
#include "MovieSceneCommonHelpers.h"
#include "Animation/AnimInstance.h"

/** 
* Gets the FaceFX component based on a given object 
* @param Object The object to get the FaceFX component from
* @returns The component if found, else nullptr
*/
UFaceFXComponent* GetFaceFXComponent(UObject* Object)
{
	UFaceFXComponent* FaceFXComponent = Cast<UFaceFXComponent>(Object);
	if (!FaceFXComponent)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			return Actor->FindComponentByClass<UFaceFXComponent>();
		}
	}
	return FaceFXComponent;
}

/** 
* Gets the section playback location for a giving sequencer playback location 
* @param Position The position on the sequencer timeline
* @param AnimSection The animation section to fetch the position for
* @returns The playback location for the given section
*/
float CalcPlaybackLocation(float Position, UFaceFXAnimationSection* AnimSection)
{
	const float AnimLength = AnimSection->GetAnimationDuration() - (AnimSection->GetStartOffset() + AnimSection->GetEndOffset());

	const float RelPosition = FMath::Clamp(Position, AnimSection->GetStartTime(), AnimSection->GetEndTime()) - AnimSection->GetStartTime();
	Position = AnimLength > 0.F ? FMath::Fmod(RelPosition, AnimLength) : 0.F;
	Position += AnimSection->GetStartOffset();
	return Position;
}

void FFaceFXAnimationTrackInstance::RestoreState(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
	{
		UObject* RuntimeObject = RuntimeObjects[i].Get();
		UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(RuntimeObject);
		if (!FaceFXComponent)
		{
			continue;
		}

		FaceFXComponent->StopAll();

		TArray<USkeletalMeshComponent*> SkelMeshTargets;
		FaceFXComponent->GetSetupSkelMeshComponents(SkelMeshTargets);
		for (USkeletalMeshComponent* SkelMeshTarget : SkelMeshTargets)
		{
			if (SkelMeshTarget->GetAnimationMode() == EAnimationMode::Type::AnimationBlueprint)
			{
				if (UAnimInstance* AnimInstance = SkelMeshTarget->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.f);
					AnimInstance->UpdateAnimation(0.f, false);
				}

				// Update space bases to reset it back to ref pose
				SkelMeshTarget->RefreshBoneTransforms();
				SkelMeshTarget->RefreshSlaveComponents();
				SkelMeshTarget->UpdateComponentToWorld();
			}
		}
	}
}

/** Stops the playback of any animation of all FaceFX components of the given runtime objects */
void StopAllPlayback(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects)
{
	for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
	{
		UObject* RuntimeObject = RuntimeObjects[i].Get();
		UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(RuntimeObject);
		if (!FaceFXComponent)
		{
			continue;
		}

		FaceFXComponent->StopAll();

		//enforce an update on the bones to trigger blend nodes
		TArray<USkeletalMeshComponent*> SkelMeshTargets;
		FaceFXComponent->GetSetupSkelMeshComponents(SkelMeshTargets);
		for (USkeletalMeshComponent* SkelMeshTarget : SkelMeshTargets)
		{
			SkelMeshTarget->RefreshBoneTransforms();
		}
	}
}

void FFaceFXAnimationTrackInstance::Update(EMovieSceneUpdateData& UpdateData, const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	check(AnimationTrack);

	//Find the section to update
	UFaceFXAnimationSection* AnimSection = Cast<UFaceFXAnimationSection>(AnimationTrack->GetSectionAtTime(UpdateData.Position));
	if (!AnimSection || !AnimSection->IsActive())
	{
		StopAllPlayback(RuntimeObjects);
		CurrentActiveSection = nullptr;
		return;
	}

	const float PlaybackLocation = CalcPlaybackLocation(UpdateData.Position, AnimSection);

	//update the current anim section within this track
	const bool IsNewAnimSection = CurrentActiveSection != AnimSection;
	CurrentActiveSection = AnimSection;
	
	//update the assigned runtime objects for this track
	for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
	{
		UObject* RuntimeObject = RuntimeObjects[i].Get();
		UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(RuntimeObject);
		if (!FaceFXComponent)
		{
			continue;
		}

		//may be null if the skel mesh component id is unset and there is no character setup on the component
		USkeletalMeshComponent* SkelMeshTarget = FaceFXComponent->GetSkelMeshTarget(AnimSection->GetComponent());

		//Always stop when we switch track keys to prevent playing animations when switching backward after the end of another track animation
		//Thats because we don't check here how long the animation actually plays and rely on the JumpTo/Play functionality alone to determine that
		if (IsNewAnimSection)
		{
			FaceFXComponent->Stop(SkelMeshTarget);
		}

		const EMovieScenePlayerStatus::Type State = Player.GetPlaybackStatus();

		const bool bLoop = UpdateData.bLooped;
		const bool bScrub = State != EMovieScenePlayerStatus::Playing;
		const bool bStop = State == EMovieScenePlayerStatus::Stopped && UpdateData.Position == UpdateData.LastPosition;

		//playing backwards or jumping
		const FFaceFXAnimId& AnimId = AnimSection->GetAnimationId();

		bool UpdateAnimation = true;

		if (State == EMovieScenePlayerStatus::Playing)
		{
			//Playback mode
			if (!IsNewAnimSection && FaceFXComponent->IsPlaying(SkelMeshTarget, RuntimeObject))
			{
				//No need to updating the FaceFXComponent
				UpdateAnimation = false;
			}
		}

		if (UpdateAnimation)
		{
			bool JumpSucceeded = false;
			if (!bStop)
			{
				//jump if not stopping
				if (AnimId.IsValid())
				{
					//play by animation id
					JumpSucceeded = FaceFXComponent->JumpToById(PlaybackLocation, bScrub, AnimId.Group, AnimId.Name, bLoop, SkelMeshTarget, RuntimeObject);
				}
				else if (UFaceFXAnim* FaceFXAnim = AnimSection->GetAnimation(FaceFXComponent))
				{
					//play by animation
					JumpSucceeded = FaceFXComponent->JumpTo(PlaybackLocation, bScrub, FaceFXAnim, bLoop, SkelMeshTarget, RuntimeObject);
				}
			}

			if (!JumpSucceeded)
			{
				//jump to failed -> i.e. out of range on non looping animation
				FaceFXComponent->Stop(SkelMeshTarget);
			}
		}

		if (SkelMeshTarget)
		{
			//enforce an update on the bones to trigger blend nodes
			SkelMeshTarget->RefreshBoneTransforms();
		}
	}
}