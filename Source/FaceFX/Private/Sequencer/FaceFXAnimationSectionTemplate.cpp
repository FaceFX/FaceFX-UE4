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

#include "Sequencer/FaceFXAnimationSectionTemplate.h"
#include "FaceFX.h"

#include "Sequencer/FaceFXAnimationTrack.h"
#include "Sequencer/FaceFXAnimationSection.h"
#include "Animation/FaceFXComponent.h"
#include "FaceFXAnim.h"

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

FFaceFXAnimationSectionTemplate::FFaceFXAnimationSectionTemplate(const UFaceFXAnimationSection* Section, const UFaceFXAnimationTrack* Track)
{
	if (Track)
	{
		SectionData.TrackId = Track->GetSignature();
	}

	if (Section)
	{
		//Copy all required section data
		SectionData.RowIndex = Section->GetRowIndex();
		SectionData.AnimationId = Section->GetAnimationId();
		SectionData.Animation = Section->GetAnimationAsset();
		SectionData.ComponentId = Section->GetComponent();
		SectionData.AnimDuration = Section->GetAnimationDuration();
		SectionData.StartOffset = Section->GetStartOffset();
		SectionData.EndOffset = Section->GetEndOffset();
		SectionData.StartTime = Section->GetStartTime();
		SectionData.EndTime = Section->GetEndTime();
	}
}

void FFaceFXAnimationSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	check(SectionData.IsValid());

	if (Context.IsSilent())
	{
		return;
	}

	//search for any token in a higher row. Lower rows take precedence. We only allow ONE section at a time per track
	const FMovieSceneTrackIdentifier TrackId = PersistentData.GetTrackKey().TrackIdentifier;

	for(int32 Idx = ExecutionTokens.Tokens.Num() - 1; Idx >= 0; --Idx)
	{
		FMovieSceneExecutionTokens::FEntry& TokenEntry = ExecutionTokens.Tokens[Idx];

		//only sort within same track
		if (TokenEntry.TrackKey.TrackIdentifier == TrackId)
		{
			const FFaceFXAnimationExecutionToken Token = (const FFaceFXAnimationExecutionToken&)TokenEntry.Token.Get(FFaceFXAnimationExecutionToken());

			//same track. Check for ordering precedence
			if (Token.GetSectionRowIndex() > SectionData.RowIndex)
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

	ExecutionTokens.Add(FFaceFXAnimationExecutionToken(SectionData));
}

void FFaceFXAnimationSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	check(SectionData.IsValid());

	FFaceFXAnimationTrackData& TrackData = PersistentData.GetOrAddTrackData<FFaceFXAnimationTrackData>();

	FObjectKey* Key = TrackData.SectionRowFaceFXComponents.Find(SectionData.RowIndex);
	if (!Key)
	{
		return;
	}

	if (UFaceFXComponent* FaceFXComponent = Cast<UFaceFXComponent>(Key->ResolveObjectPtr()))
	{
		//check if the currently active section for this track is actually the active one
		if (TrackData.ActiveSectionRowIndex != SectionData.RowIndex)
		{
			//ignore as we're playing another section right now
			return;
		}

		FFaceFXAnimId AnimId = SectionData.AnimationId;

		//determine animation, either by ID of by asset
		if (!AnimId.IsValid())
		{
			if (const UFaceFXAnim* Anim = UFaceFXAnimationSection::GetAnimation(SectionData.Animation, FaceFXComponent))
			{
				AnimId = Anim->GetId();
			}
		}

		//Only stop if that's current animation active in the FaceFX component
		USkeletalMeshComponent* SkelMeshTarget = FaceFXComponent->GetSkelMeshTarget(SectionData.ComponentId);
		if (FaceFXComponent->IsAnimationActive(AnimId, SkelMeshTarget))
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

void FFaceFXAnimationExecutionToken::Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
{
	check(SectionData.IsValid());

	//update track data
	FFaceFXAnimationTrackData& TrackData = PersistentData.GetOrAddTrackData<FFaceFXAnimationTrackData>();
	const bool IsNewAnimSection = TrackData.ActiveSectionRowIndex != SectionData.RowIndex;
	TrackData.ActiveSectionRowIndex = SectionData.RowIndex;

	const EMovieScenePlayerStatus::Type State = Context.GetStatus();

	const float Position = Context.GetTime();
	const float LastPosition = Context.GetPreviousTime();
	const float PlaybackLocation = UFaceFXAnimationSection::GetPlaybackLocation(Position, SectionData.AnimDuration, SectionData.StartOffset, SectionData.EndOffset, SectionData.StartTime, SectionData.EndTime);

	//during reverse playback we use scrubbing instead, as reverse playback on audio component/FaceFX is not supported
	const bool bIsReversePlayback = Context.GetDirection() == EPlayDirection::Backwards;
	const bool bScrub = State != EMovieScenePlayerStatus::Playing || bIsReversePlayback;
	const bool bPaused = State == EMovieScenePlayerStatus::Stopped && Position == LastPosition;

	for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
	{
		if (UObject* RuntimeObject = Object.Get())
		{
			UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(RuntimeObject);
			if (!FaceFXComponent)
			{
				continue;
			}

			TrackData.SectionRowFaceFXComponents.Add(SectionData.RowIndex, FaceFXComponent);

			const bool IsUseIdBasedAnim = SectionData.AnimationId.IsValid();

			//FaceFX animation set for the section. Only used if the FaceFXAnimId is not set
			UFaceFXAnim* FaceFXAnim = IsUseIdBasedAnim ? nullptr : UFaceFXAnimationSection::GetAnimation(SectionData.Animation, FaceFXComponent);
			if (!IsUseIdBasedAnim && !FaceFXAnim)
			{
				//animation not found
				continue;
			}

			//may be null if the skel mesh component id is unset and there is no character setup on the component
			USkeletalMeshComponent* SkelMeshTarget = FaceFXComponent->GetSkelMeshTarget(SectionData.ComponentId);

			//check if we currently play the animation specified by this section (either by ID or asset). New sections are considered as animation changes as well
			const bool IsAnimChanged = IsNewAnimSection || !FaceFXComponent->IsAnimationActive(IsUseIdBasedAnim ? SectionData.AnimationId : FaceFXAnim->GetId(), SkelMeshTarget, RuntimeObject);

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
				if (!IsAnimChanged && FaceFXComponent->IsPlayingAnimation(IsUseIdBasedAnim ? SectionData.AnimationId : FaceFXAnim->GetId(), SkelMeshTarget, RuntimeObject))
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
					if (IsUseIdBasedAnim)
					{
						//play by animation id
						JumpSucceeded = FaceFXComponent->JumpToById(PlaybackLocation, bScrub, SectionData.AnimationId.Group, SectionData.AnimationId.Name, false, SkelMeshTarget, RuntimeObject);
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