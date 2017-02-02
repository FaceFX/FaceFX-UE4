/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2017 OC3 Entertainment, Inc.
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
#include "Sequencer/FaceFXAnimationSectionTemplate.h"

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

void FFaceFXAnimationSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	check(AnimationTrack);
	check(AnimationSection);

	if (Context.IsSilent())
	{
		return;
	}

	//search for any token in a higher row. Lower rows take precedence. We only allow ONE section at a time per track
	const UFaceFXAnimationTrack* SectionTrack = AnimationSection->GetTrack();
	const FGuid SectionTrackId = SectionTrack ? SectionTrack->GetSignature() : FGuid();

	for(int32 Idx = ExecutionTokens.Tokens.Num() - 1; Idx >= 0; --Idx)
	{
		const FFaceFXAnimationExecutionToken Token = (const FFaceFXAnimationExecutionToken&)ExecutionTokens.Tokens[Idx].Token.Get(FFaceFXAnimationExecutionToken());

		if (Token.GetSectionTrackId() == SectionTrackId)
		{
			//same track. Check for ordering precedence
			if (Token.GetSectionRowIndex() > AnimationSection->GetRowIndex())
			{
				ExecutionTokens.Tokens.RemoveAtSwap(Idx);
			}
			else
			{
				//keep existing token
				return;
			}
		}
	}

	ExecutionTokens.Add(FFaceFXAnimationExecutionToken(AnimationSection));
}

void FFaceFXAnimationSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	check(AnimationSection);

	if (UObject* Actor = AnimationSection->GetActor())
	{
		UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(Actor);
		if (!FaceFXComponent)
		{
			return;
		}

		//check if the currently active section for this track is actually the active one
		FFaceFXAnimationTrackData& TrackData = PersistentData.GetOrAddTrackData<FFaceFXAnimationTrackData>();
		if (TrackData.ActiveSection != AnimationSection)
		{
			//ignore as we're playing another section right now
			return;
		}

		//determine animation, either by ID of by asset
		FFaceFXAnimId FaceFXAnimId = AnimationSection->GetAnimationId();
		if (!FaceFXAnimId.IsValid())
		{
			if (UFaceFXAnim* Anim = AnimationSection->GetAnimation(FaceFXComponent))
			{
				FaceFXAnimId = Anim->GetId();
			}
		}

		//Only stop if that's current animation active in the FaceFX component
		USkeletalMeshComponent* SkelMeshTarget = FaceFXComponent->GetSkelMeshTarget(AnimationSection->GetComponent());
		if (FaceFXComponent->IsAnimationActive(FaceFXAnimId, SkelMeshTarget, Actor))
		{
			FaceFXComponent->Stop(SkelMeshTarget);

			if (SkelMeshTarget)
			{
				//enforce an update on the bones to trigger blend nodes
				SkelMeshTarget->RefreshBoneTransforms();
			}
		}
	}
}

int32 FFaceFXAnimationExecutionToken::GetSectionRowIndex() const
{
	if (AnimationSection)
	{
		return AnimationSection->GetRowIndex();
	}
	return INDEX_NONE;
}

FGuid FFaceFXAnimationExecutionToken::GetSectionTrackId() const
{
	if (AnimationSection)
	{
		if (UFaceFXAnimationTrack* Track = AnimationSection->GetTrack())
		{
			return Track->GetSignature();
		}
	}
	return FGuid();
}

void FFaceFXAnimationExecutionToken::Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
{
	check(AnimationSection);

	//update track data
	FFaceFXAnimationTrackData& TrackData = PersistentData.GetOrAddTrackData<FFaceFXAnimationTrackData>();
	const bool IsNewAnimSection = TrackData.ActiveSection != AnimationSection;
	TrackData.ActiveSection = AnimationSection;

	const EMovieScenePlayerStatus::Type State = Context.GetStatus();

	const float Position = Context.GetTime();
	const float LastPosition = Context.GetPreviousTime();
	const float PlaybackLocation = AnimationSection->GetPlaybackLocation(Position);

	//during reverse playback we use scrubbing instead, as reverse playback on audio component/FaceFX is not supported
	const bool bIsReversePlayback = Context.GetDirection() == EPlayDirection::Backwards;
	const bool bScrub = State != EMovieScenePlayerStatus::Playing || bIsReversePlayback;
	const bool bPaused = State == EMovieScenePlayerStatus::Stopped && Position == LastPosition;

	//the id of the section if id based. If its asset based, this is invalid
	const FFaceFXAnimId& FaceFXAnimId = AnimationSection->GetAnimationId();
	
	for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
	{
		if (UObject* RuntimeObject = Object.Get())
		{
			UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(RuntimeObject);
			if (!FaceFXComponent)
			{
				continue;
			}

			//FaceFX animation set for the section. Only used if the FaceFXAnimId is not set
			UFaceFXAnim* FaceFXAnim = FaceFXAnimId.IsValid() ? nullptr : AnimationSection->GetAnimation(FaceFXComponent);

			//may be null if the skel mesh component id is unset and there is no character setup on the component
			USkeletalMeshComponent* SkelMeshTarget = FaceFXComponent->GetSkelMeshTarget(AnimationSection->GetComponent());

			//check if we currently play the animation specified by this section (either by ID or asset). New sections are considered as animation changes as well
			const bool IsAnimChanged = IsNewAnimSection || !FaceFXComponent->IsAnimationActive(FaceFXAnimId.IsValid() ? FaceFXAnimId : FaceFXAnim->GetId(), SkelMeshTarget, RuntimeObject);

			//Always stop when we switch track keys to prevent playing animations when switching backward after the end of another track animation
			//Thats because we don't check here how long the animation actually plays and rely on the JumpTo/Play functionality alone to determine that
			if (IsAnimChanged)
			{
				FaceFXComponent->Stop(SkelMeshTarget);
			}

			bool UpdateAnimation = true;

			if (State == EMovieScenePlayerStatus::Playing && !bIsReversePlayback)
			{
				//Playback mode. Check if we currently play this animation already
				if (!IsAnimChanged && FaceFXComponent->IsPlayingAnimation(FaceFXAnimId.IsValid() ? FaceFXAnimId : FaceFXAnim->GetId(), SkelMeshTarget, RuntimeObject))
				{
					//No need to updating the FaceFXComponent when we already play this animation
					UpdateAnimation = false;
				}
			}

			if (UpdateAnimation)
			{
				bool JumpSucceeded = false;
				if (bPaused)
				{
					FaceFXComponent->Pause(SkelMeshTarget);
				}
				else
				{
					//jump if not stopping
					if (FaceFXAnimId.IsValid())
					{
						//play by animation id
						JumpSucceeded = FaceFXComponent->JumpToById(PlaybackLocation, bScrub, FaceFXAnimId.Group, FaceFXAnimId.Name, false, SkelMeshTarget, RuntimeObject);
					}
					else if (FaceFXAnim)
					{
						//play by animation
						JumpSucceeded = FaceFXComponent->JumpTo(PlaybackLocation, bScrub, FaceFXAnim, false, SkelMeshTarget, RuntimeObject);
					}

					if (!JumpSucceeded)
					{
						//jump to failed -> i.e. out of range on non looping animation
						FaceFXComponent->Stop(SkelMeshTarget);
					}
				}
			}

			if (SkelMeshTarget)
			{
				//enforce an update on the bones to trigger blend nodes
				SkelMeshTarget->RefreshBoneTransforms();
			}
		}
	}
}