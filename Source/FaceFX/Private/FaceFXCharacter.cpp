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
#include "FaceFXContext.h"

#include "GameFramework/Actor.h"
#include "Animation/FaceFXComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"

DECLARE_CYCLE_STAT(TEXT("Tick FaceFX Character"), STAT_FaceFXTick, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Update FaceFX Transforms"), STAT_FaceFXUpdateTransforms, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Load FaceFX Assets"), STAT_FaceFXLoad, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Play FaceFX"), STAT_FaceFXPlay, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("FaceFX Events"), STAT_FaceFXEvents, STATGROUP_FACEFX);

/**
* Checks the FaceFX runtime execution result and the errno for errors
* @param Value The return value returned from the FaceFX call
* @returns True if call succeeded, else false
*/
bool Check(int32 Value)
{
	return Value != 0 && (ffx_errno() == EOK);
}

FOnFaceFXCharacterPlayAssetIncompatibleSignature UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible;

UFaceFXCharacter::UFaceFXCharacter(const class FObjectInitializer& PCIP) : Super(PCIP),
	ActorHandle(nullptr),
	FrameState(nullptr),
	BoneSetHandle(nullptr),
	CurrentAnimHandle(nullptr),
	CurrentTime(.0F),
	CurrentAnimProgress(0.F),
	CurrentAnimDuration(.0F),
	CurrentAudioProgress(0.F),
	AnimPlaybackState(EPlaybackState::Stopped),
	AudioPlaybackState(EPlaybackState::Stopped),
	bIsDirty(true),
	bIsLooping(false),
	bCanPlay(true),
	bIsAutoPlaySound(false),
	bDisabledMorphTargets(false)
#if WITH_EDITOR
	,LastFrameNumber(0)
#endif
{
#if WITH_EDITOR
	if(!IsTemplate())
	{
		OnFaceFXAnimChangedHandle = UFaceFXCharacter::OnAssetChanged.AddUObject(this, &UFaceFXCharacter::OnFaceFXAssetChanged);
	}
#endif
}

void UFaceFXCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	bCanPlay = false;
	Reset();

#if WITH_EDITOR
	UFaceFXCharacter::OnAssetChanged.Remove(OnFaceFXAnimChangedHandle);
#endif
}

bool UFaceFXCharacter::TickUntil(float Duration, bool& OutAudioStarted)
{
	if(!bCanPlay || Duration < 0.F)
	{
		return false;
	}
	
	CurrentTime = 0.F;
	CurrentAnimProgress = 0.F;

	/** The steps to perform */
	static const float TickSteps = 0.1F;

	CurrentTime = Duration;

	const bool ProcessZeroSuccess = Check(ffx_process_frame(ActorHandle, FrameState, 0.F));
	const bool bIsAudioStartedAtZero = IsAudioStarted();

	if (!ProcessZeroSuccess || !Check(ffx_process_frame(ActorHandle, FrameState, CurrentTime)))
	{
		//update failed
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::TickUntil. FaceFX call <ffx_process_frame> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(bIsAudioStartedAtZero || IsAudioStarted())
	{
		OutAudioStarted = true;
	}	

	CurrentAnimProgress = CurrentTime - CurrentAnimStart;
	bIsDirty = true;

	ProcessMorphTargets();
	
	return true;
}

void UFaceFXCharacter::Tick(float DeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FaceFXTick);

	const bool IsNonZeroTick = DeltaTime > 0.F;
	checkf(!IsNonZeroTick || bCanPlay, TEXT("Internal Error: FaceFX character is not allowed to tick."));

#if WITH_EDITOR
	if(LastFrameNumber == GFrameNumber)
	{
		//tick was called twice within the same frame -> happens when running in PIE with dedicated server active or multiple instances
		return;
	}
	LastFrameNumber = GFrameNumber;
#endif

	//progress in time
	CurrentTime += DeltaTime;
	CurrentAnimProgress += DeltaTime;

	if(IsPlayingAudio())
	{
		CurrentAudioProgress += DeltaTime;
	}

	if(IsNonZeroTick && CurrentAnimProgress >= CurrentAnimDuration)
	{
		//end of animation reached
		if(IsLooping())
		{
			Restart();
		}
		else
		{
			Stop();
		}
	}

	if (!Check(ffx_process_frame(ActorHandle, FrameState, CurrentTime)))
	{
		//update failed
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Tick. FaceFX call <ffx_process_frame> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return;
	}

	if(IsAudioStarted())
	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
		UAudioComponent* AudioCompStartedOn = nullptr;
		const bool AudioStarted = PlayAudio(&AudioCompStartedOn);
		
		OnPlaybackStartAudio.Broadcast(this, CurrentAnim, AudioStarted, AudioCompStartedOn);					
	}

	ProcessMorphTargets();

	bIsDirty = true;
}

bool UFaceFXCharacter::IsTickable() const
{
	return IsPlaying() && bCanPlay;
}

TStatId UFaceFXCharacter::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFaceFXCharacter, STATGROUP_Tickables);
}

bool UFaceFXCharacter::GetAnimationBounds(const UFaceFXAnim* Animation, float& OutStart, float& OutEnd)
{
	ffx_anim_handle_t* AnimHandle = LoadAnimation(Animation->GetData());
	if(!AnimHandle)
	{
		return false;
	}

	bool Result = true;
	if(!Check(ffx_get_anim_bounds(AnimHandle, &OutStart, &OutEnd)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBounds. FaceFX call <ffx_get_anim_bounds> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(Animation));
		Result = false;
	}

	ffx_destroy_anim_handle(&AnimHandle, nullptr, nullptr);
	AnimHandle = nullptr;	

	return Result;
}

#if FACEFX_USEANIMATIONLINKAGE
bool UFaceFXCharacter::GetAnimationBoundsById(const AActor* Actor, const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd)
{
	if(Actor)
	{
		if(const UFaceFXComponent* FaceFXComp = Actor->FindComponentByClass<UFaceFXComponent>())
		{
			if(const UFaceFXCharacter* FxCharacter = FaceFXComp->GetCharacter())
			{
				return FxCharacter->GetAnimationBoundsById(AnimId, OutStart, OutEnd);
			}
		}
	}
	return false;
}

bool UFaceFXCharacter::GetAnimationBoundsById(const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd) const
{
	if(!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBoundsById. FaceFX asset not loaded."));
		return false;
	}

	if(const UFaceFXAnim* Anim = FaceFXActor->GetAnimation(AnimId))
	{
		return GetAnimationBounds(Anim, OutStart, OutEnd);
	}
	else
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBoundsById. Animation does not exist. Group=%s, Anim=%s. Asset: %s"), *AnimId.Group.GetPlainNameString(), *AnimId.Name.GetPlainNameString(), *GetNameSafe(FaceFXActor));
	}

	return false;
}

bool UFaceFXCharacter::GetAllLinkedAnimationIds(TArray<FFaceFXAnimId>& OutAnimIds) const
{
	if(!FaceFXActor)
	{
		return false;
	}

	FaceFXActor->GetAnimationIds(OutAnimIds);
	return true;
}

#endif //FACEFX_USEANIMATIONLINKAGE

bool UFaceFXCharacter::GetAnimationBounds(float& OutStart, float& OutEnd) const
{
	if(!IsPlaying() || !CurrentAnimHandle)
	{
		return false;
	}

	//get anim bounds
	if(!Check(ffx_get_anim_bounds(CurrentAnimHandle, &OutStart, &OutEnd)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBounds. FaceFX call <ffx_get_anim_bounds> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	return true;
}

#if FACEFX_USEANIMATIONLINKAGE

bool UFaceFXCharacter::Play(const FFaceFXAnimId& AnimId, bool Loop)
{
	if(!AnimId.IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. Invalid animation name/group. Group=%s, Anim=%s. Asset: %s"), *AnimId.Group.GetPlainNameString(), *AnimId.Name.GetPlainNameString(), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX asset not loaded."));
		return false;
	}

	if(const UFaceFXAnim* Anim = FaceFXActor->GetAnimation(AnimId))
	{
		return Play(Anim, Loop);
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. Unknown animation name/group. Group=%s, Anim=%s. Asset: %s"), *AnimId.Group.GetPlainNameString(), *AnimId.Name.GetPlainNameString(), *GetNameSafe(FaceFXActor));
	return false;
}

#endif //FACEFX_USEANIMATIONLINKAGE

bool UFaceFXCharacter::Play(const UFaceFXAnim* Animation, bool Loop)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXPlay);

	if(!Animation)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX animation asset missing."));
		return false;
	}

	if(!Animation->IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX animation asset is in an invalid state. Please reimport that asset. Asset: %s"), *GetNameSafe(Animation));
		return false;
	}

	if(!bCanPlay)
	{
		return false;
	}

	if(!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX asset not loaded."));
		return false;
	}

	if(!IsLoaded())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(IsPlayingOrPaused())
	{
		if(CurrentAnim != Animation->GetId())
		{
			//warn only about any animation that is not getting restarted
			UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Play. Animation already playing/paused. Stopping now. Actor: %s. Animation: %s"), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		}
		Stop();
	}

	if(CurrentAnim != Animation->GetId())
	{
		//animation changed -> create new handle

		//check if we actually can play this animation
		ffx_anim_handle_t* NewAnimHandle = LoadAnimation(Animation->GetData());
		if(!IsCanPlay(NewAnimHandle))
		{
			UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Play. Animation is not compatible with FaceFX actor handle. Actor: %s. Animation: %s"), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
			OnFaceFXCharacterPlayAssetIncompatible.Broadcast(this, Animation);
			return false;
		}

		//unload previous animation
		UnloadCurrentAnim();

		CurrentAnimHandle = NewAnimHandle;

		PrepareAudio(Animation);
	}

	//get anim bounds
	float AnimStart = .0F;
	float AnimEnd = .0F;
	if(!Check(ffx_get_anim_bounds(CurrentAnimHandle, &AnimStart, &AnimEnd)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call <ffx_get_anim_bounds> failed. %s. Actor: %s. Animation: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	//sanity check
	CurrentAnimDuration = AnimEnd - AnimStart;
	if(CurrentAnimDuration <= .0F)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. Invalid animation length of %.3f seconds. Actor: %s. Animation: %s"), CurrentAnimDuration, *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	if(!Check(ffx_play(ActorHandle, CurrentAnimHandle, nullptr)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call <ffx_play> failed. %s. Actor: %s. Animation: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	//reset timers and states
	CurrentAnimProgress = 0.F;
	CurrentAnim = Animation->GetId();
	CurrentAnimStart = AnimStart;
	AnimPlaybackState = EPlaybackState::Playing;
	bIsLooping = Loop;

	//tick once so we update the transforms at least once (in case another animation was playing before and we pause directly after play)
	EnforceZeroTick();

	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
		OnPlaybackStarted.Broadcast(this, CurrentAnim);
	}	

	return true;
}

bool UFaceFXCharacter::Resume()
{
	if(!bCanPlay)
	{
		return false;
	}

	if(!IsLoaded())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Resume. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(IsPlaying())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Resume. Animation still playing. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(!Check(ffx_resume(ActorHandle, CurrentTime)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Resume. FaceFX call <ffx_resume> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}
	
	AnimPlaybackState = EPlaybackState::Playing;
	ResumeAudio();

	return true;
}

bool UFaceFXCharacter::Pause()
{
	if(!IsLoaded())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Pause. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (IsPlaying() && !Check(ffx_pause(ActorHandle, CurrentTime)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Pause. FaceFX call <ffx_pause> failed. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	AnimPlaybackState = EPlaybackState::Paused;
	PauseAudio();

	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
		OnPlaybackPaused.Broadcast(this, CurrentAnim);
	}

	return true;
}

bool UFaceFXCharacter::Stop()
{
	if(!IsLoaded())
	{
		//not loaded yet
		return false;
	}

	const bool WasPlayingOrPaused = IsPlayingOrPaused();
	if(WasPlayingOrPaused)
	{
		if (!Check(ffx_stop(ActorHandle)))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Stop. FaceFX call <ffx_stop> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
			return false;
		}

		//enforce a processing so we reset to the initial transforms on the next update
		EnforceZeroTick();
	}

	const FFaceFXAnimId StoppedAnimId = CurrentAnim;

	//reset timer and audio query indicator
	CurrentAnimProgress = .0F;
	CurrentAnim.Reset();
	AnimPlaybackState = EPlaybackState::Stopped;
	StopAudio();

	UnloadCurrentAnim();

	if(WasPlayingOrPaused)
	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
		OnPlaybackStopped.Broadcast(this, StoppedAnimId);
	}

	return true;
}

bool UFaceFXCharacter::JumpTo(float Position)
{
	if(Position < 0.F || (!IsLooping() && Position > CurrentAnimDuration))
	{
		return false;
	}

	if(!bCanPlay)
	{
		return false;
	}

	if(!IsLoaded())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::JumpTo. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(!CurrentAnimHandle)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. Current animation is invalid. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	//explicitly stop and start the playback again
	if (!Check(ffx_stop(ActorHandle)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. FaceFX call <ffx_stop> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}
	
	AnimPlaybackState = EPlaybackState::Stopped;

	//play again
	if(!Check(ffx_play(ActorHandle, CurrentAnimHandle, nullptr)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. FaceFX call <ffx_play> failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	AnimPlaybackState = EPlaybackState::Playing;
	
	if(Position > CurrentAnimDuration)
	{
		//cap the position to the animation range
		Position = FMath::Fmod(Position, CurrentAnimDuration);
	}

	bool IsAudioStarted = false;
	if(TickUntil(Position, IsAudioStarted) && IsAudioStarted)
	{
		const float AudioPosition = Position + CurrentAnimStart;
		checkf(AudioPosition >= 0.F, TEXT("Invalid audio playback range."));
		PlayAudio(AudioPosition);
	}
	else
	{
		StopAudio();
	}

	return true;
}

void UFaceFXCharacter::Reset()
{
	//Stop any playing animation before destroying the handles
	Stop();

	//free the facefx handles
	UnloadCurrentAnim();

	if (ActorHandle) 
	{
		ffx_destroy_actor_handle(&ActorHandle, nullptr, nullptr);
		ActorHandle = nullptr;
	}

	if(FrameState)
	{
		ffx_destroy_frame_state(&FrameState);
		FrameState = nullptr;
	}

	if (BoneSetHandle)
	{
		ffx_destroy_bone_set_handle(&BoneSetHandle, nullptr, nullptr);
		BoneSetHandle = nullptr;
	}

	//reset arrays
	XForms.Empty();
	BoneTransforms.Empty();
	BoneIds.Empty();

	MorphTargetNames.Empty();
	MorphTargetTrackValues.Empty();

	FaceFXActor = nullptr;

	bIsDirty = true;
}

bool UFaceFXCharacter::IsPlayingOrPaused(const class UFaceFXAnim* Animation) const
{
	return Animation && IsPlayingOrPaused(Animation->GetId());
}

bool UFaceFXCharacter::Load(const UFaceFXActor* Dataset, bool IsDisabledMorphTargets)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXLoad);

	if(!Dataset)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Missing FaceFXActor asset."));
		return false;
	}

	if(!Dataset->IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Invalid FaceFXActor asset. Please reimport that asset. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	Reset();

	FaceFXActor = Dataset;

	const FFaceFXActorData& ActorData = FaceFXActor->GetData();

	ffx_context_t Context = FFaceFXContext::CreateContext();

    //only create the bone set handle if there is bone set data
    if(ActorData.BonesRawData.Num() > 0)
    {
	    if (!Check(ffx_create_bone_set_handle((char*)(&ActorData.BonesRawData[0]), ActorData.BonesRawData.Num(), FFX_RUN_INTEGRITY_CHECK, FFX_USE_FULL_XFORMS, &BoneSetHandle, &Context))) 
	    {
		    UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX bone handle. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		    Reset();
		    return false;
	    }
    }

    //make sure there is actor data
    if(ActorData.ActorRawData.Num() == 0)
    {
        UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. No FaceFX actor data present. Asset: %s"), *GetNameSafe(FaceFXActor));
        Reset();
        return false;
    }

	static size_t channel_count  = FACEFX_CHANNELS;
	if (!Check(ffx_create_actor_handle((char*)(&ActorData.ActorRawData[0]), ActorData.ActorRawData.Num(), FFX_RUN_INTEGRITY_CHECK, channel_count, &ActorHandle, &Context)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX actor handle. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	if (!Check(ffx_create_frame_state(ActorHandle, &FrameState, &Context)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX state. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

    if(BoneSetHandle)
    {
	    size_t XFormCount = 0;
	    if (!Check(ffx_get_bone_set_bone_count(BoneSetHandle, &XFormCount)))
	    {
		    UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to receive FaceFX bone count. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		    Reset();
		    return false;
	    }


	    if(XFormCount > 0)
	    {
		    //prepare buffer
		    XForms.AddUninitialized(XFormCount);
		
		    //retrieve bone names
		    BoneIds.AddUninitialized(XFormCount);

		    if(!Check(ffx_get_bone_set_bone_ids(BoneSetHandle, reinterpret_cast<uint64_t*>(BoneIds.GetData()), XFormCount)))
		    {
			    UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to receive FaceFX bone names. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
			    Reset();
			    return false;
		    }

			//prepare transform buffer
			BoneTransforms.AddZeroed(XFormCount);

			//match bone ids with names from the .ffxids assets
			for(const uint64& BoneIdHash : BoneIds)
			{
				if(const FFaceFXIdData* BoneId = ActorData.Ids.FindByKey(BoneIdHash))
				{
					BoneNames.Add(BoneId->Name);
				}
				else
				{
					UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Load. Unknown bone id. %i. Asset: %s"), BoneIdHash, *GetNameSafe(FaceFXActor));
				}
			}
	    }
    }

	bDisabledMorphTargets = IsDisabledMorphTargets;
	if(!bDisabledMorphTargets && !SetupMorphTargets())
	{
		Reset();
		return false;
	}

	return true;
}

void UFaceFXCharacter::ProcessMorphTargets()
{
	checkf(MorphTargetNames.Num() == MorphTargetTrackValues.Num(), TEXT("Morph target names indices must match the track values buffer"));

	const int32 MorphTargetsToProcess = MorphTargetTrackValues.Num();
	if(MorphTargetsToProcess == 0)
	{
		//no morph target to process
		return;
	}

	//read track values
	if(!Check(ffx_read_frame_track_values(FrameState, &MorphTargetTrackValues[0], MorphTargetsToProcess)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::ProcessMorphTargets. Unable to process morph targets. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return;
	}

	//apply morph target track values
	if(USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent())
	{
		for(int32 Idx = 0; Idx < MorphTargetsToProcess; ++Idx)
		{
			SkelMeshComp->SetMorphTarget(MorphTargetNames[Idx], MorphTargetTrackValues[Idx].value);
		}
	}
	else
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::ProcessMorphTargets. Unable to find owners skel mesh component. Actor: %s. Asset: %s"), *GetNameSafe(GetOwningActor()), *GetNameSafe(FaceFXActor));
		return;
	}
}

bool UFaceFXCharacter::SetupMorphTargets()
{
	check(IsLoaded());

	//reset morph target data
	MorphTargetNames.Reset();
	MorphTargetTrackValues.Reset();

	if(USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent())
	{
		const int32 NumMorphTargets = SkelMeshComp->SkeletalMesh ? SkelMeshComp->SkeletalMesh->MorphTargetIndexMap.Num() : 0;
		if(NumMorphTargets == 0)
		{
			//no morph targets in skel mesh
			return true;
		}
		
		//the lookup for the created facefx ids and the morph target names
		TMap<uint64_t, FName> NameLookUp;
		NameLookUp.Reserve(NumMorphTargets);

		TArray<ffx_id_index_t> TrackIndices;
		TrackIndices.Reserve(NumMorphTargets);

		for(auto It = SkelMeshComp->SkeletalMesh->MorphTargetIndexMap.CreateConstIterator(); It; ++It)
		{
			const FName& MorphTarget = It.Key();

			ffx_id_index_t IdIndex;
			if(!Check(ffx_create_id(TCHAR_TO_ANSI(*MorphTarget.ToString()), &IdIndex.id)))
			{
				UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::SetupMorphTargets. Unable to create FaceFX id for FaceFX track <%s>. %s. Asset: %s"), *MorphTarget.ToString(), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
				return false;
			}

			TrackIndices.Add(IdIndex);
			NameLookUp.Add(IdIndex.id, MorphTarget);
		}

		//fetch the track indices by their facefx ids generated for the morph target names
		if(!Check(ffx_find_tracks_in_actor_by_id(ActorHandle, &TrackIndices[0], NumMorphTargets)))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::SetupMorphTargets. Unable to fetch FaceFX track indices. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
			return false;
		}

		//sort by track indices for FaceFX runtime performance reasons
		TrackIndices.Sort([](const ffx_id_index_t& A, const ffx_id_index_t& B) 
		{
			return A.index < B.index;
		});

		//map back track indices with morph target names
		for(const ffx_id_index_t& TrackIndex : TrackIndices)
		{
			//lookup name to map track index back to the originating morph target
			const FName& MorphTarget = NameLookUp.FindChecked(TrackIndex.id);

			if(TrackIndex.index == INDEX_NONE)
			{
				//Track was not found -> ignore morph target
				UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::SetupMorphTargets. No FaceFX track found for SkelMesh Morph Target <%s>. SkelMesh: &s. Asset: %s"), 
					*MorphTarget.ToString(), *GetNameSafe(SkelMeshComp->SkeletalMesh), *GetNameSafe(FaceFXActor));
				continue;
			}

			//store the morph target names
			MorphTargetNames.Add(MorphTarget);

			//prepare the track value buffer
			ffx_track_value_t NewTrackValue;
			NewTrackValue.index = TrackIndex.index;
			MorphTargetTrackValues.Add(NewTrackValue);
		}
	}
	
	checkf(MorphTargetNames.Num() == MorphTargetTrackValues.Num(), TEXT("Morph target names indices must match the track values buffer"));
	return true;
}

bool UFaceFXCharacter::IsCanPlay(const UFaceFXAnim* Animation) const
{
	return Animation && IsCanPlay(LoadAnimation(Animation->GetData()));
}

bool UFaceFXCharacter::IsCanPlay(ffx_anim_handle_t* AnimationHandle) const
{
	return ActorHandle && AnimationHandle && Check(ffx_check_actor_compatibility_with_anim(ActorHandle, AnimationHandle));
}

bool UFaceFXCharacter::IsAudioStarted()
{
	int ChannelFlags[FACEFX_CHANNELS];
	if(Check(ffx_read_frame_channel_flags(FrameState, ChannelFlags, FACEFX_CHANNELS)))
	{
		return (ChannelFlags[0] & FFX_START_AUDIO) != 0;
	}
	else
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::IsAudioStarted. Retrieving channel states for StartAudio event failed. %s"), *GetFaceFXError());
	}
	return false;
}

int32 UFaceFXCharacter::GetBoneNameTransformIndex(const FName& Name) const
{
	if(const FFaceFXIdData* BoneId = FaceFXActor ? FaceFXActor->GetData().Ids.FindByKey(Name) : nullptr)
	{
		//get index of the FaceFX bone
		int32 FaceFXBoneIdx;
		if(BoneIds.Find(BoneId->Id, FaceFXBoneIdx))
		{
			return FaceFXBoneIdx;
		}
	}
	return INDEX_NONE;
}

void UFaceFXCharacter::UnloadCurrentAnim()
{
	if (CurrentAnimHandle)
	{
		ffx_destroy_anim_handle(&CurrentAnimHandle, nullptr, nullptr);
		CurrentAnimHandle = nullptr;
	}
}

ffx_anim_handle_t* UFaceFXCharacter::LoadAnimation(const FFaceFXAnimData& AnimData)
{
	//load animations
	if(AnimData.RawData.Num() == 0)
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::LoadAnimation. Missing facefx animation data."));
		return nullptr;
	}

	ffx_context_t Context = FFaceFXContext::CreateContext();

	ffx_anim_handle_t* NewHandle = nullptr;
	if (!Check(ffx_create_anim_handle((char*)(&AnimData.RawData[0]), AnimData.RawData.Num(), FFX_RUN_INTEGRITY_CHECK, &NewHandle, &Context)) || !NewHandle)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::LoadAnimation. Unable to create animation FaceFX handle. %s"), *GetFaceFXError());
	}
	
	return NewHandle;
}

USkeletalMeshComponent* UFaceFXCharacter::GetOwningSkelMeshComponent() const
{
	const UFaceFXComponent* FaceFXComp = GetOwningFaceFXComponent();
	return FaceFXComp ? FaceFXComp->GetSkelMeshTarget(this) : nullptr;
}

UFaceFXComponent* UFaceFXCharacter::GetOwningFaceFXComponent() const
{
	return Cast<UFaceFXComponent>(GetOuter());
}

AActor* UFaceFXCharacter::GetOwningActor() const
{
	const UFaceFXComponent* OwnerComp = Cast<UFaceFXComponent>(GetOuter());
	return OwnerComp ? OwnerComp->GetOwner() : nullptr;
}

UAudioComponent* UFaceFXCharacter::GetAudioComponent() const
{
	if(AudioComponent)
	{
		return AudioComponent;
	}
	AActor* Owner = GetOwningActor();
	return Owner ? Owner->FindComponentByClass<UAudioComponent>() : nullptr;
}

void UFaceFXCharacter::PrepareAudio(const UFaceFXAnim* Animation)
{
	check(Animation);
	
	CurrentAnimSound = bIsAutoPlaySound ? Animation->GetAudio() : nullptr;
	
	if(bIsAutoPlaySound && !CurrentAnimSound.IsValid() && CurrentAnimSound.ToStringReference().IsValid())
	{
		//asset not loaded yet -> async load to have it (hopefully) ready when the FaceFX runtime audio start event triggers
		TArray<FStringAssetReference> StreamingRequests;
		StreamingRequests.Add(CurrentAnimSound.ToStringReference());
		FaceFX::GetStreamer().RequestAsyncLoad(StreamingRequests, FStreamableDelegate());
	}
}

bool UFaceFXCharacter::PlayAudio(float Position, UAudioComponent** OutAudioComp)
{
	if(bIsAutoPlaySound && CurrentAnimSound.ToStringReference().IsValid())
	{
		UAudioComponent* AudioComp = GetAudioComponent();

		if(!AudioComp)
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PlayAudio. Playing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s. Asset: %s")
				, *GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToStringReference().ToString());
			return false;
		}

		USoundWave* Sound = CurrentAnimSound.Get();
		if(!Sound)
		{
			//sound not loaded (yet) -> load sync now. Here we could also use a delayed audio playback system
			Sound = Cast<USoundWave>(StaticLoadObject(USoundWave::StaticClass(), this, *CurrentAnimSound.ToStringReference().ToString()));
		}

		if(Sound)
		{
			CurrentAudioProgress = FMath::Clamp(Position, 0.F, Sound->GetDuration());
			AudioComp->SetSound(Sound);
			AudioComp->Play(CurrentAudioProgress);			

			if(OutAudioComp)
			{
				*OutAudioComp = AudioComp;
			}
			AudioPlaybackState = AudioComp->IsPlaying() ? EPlaybackState::Playing : EPlaybackState::Stopped;
			return IsPlayingAudio();
		}
		else
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PlayAudio. Playing audio failed. Audio asset failed to load. Actor: %s. Asset: %s"), 
				*GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToStringReference().ToString());
		}
	}

	return false;
}

bool UFaceFXCharacter::PauseAudio()
{
	if(!IsPlayingAudio())
	{
		//nothing playing right now
		return true;
	}

	AudioPlaybackState = EPlaybackState::Paused;

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->Stop();
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PauseAudio. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool UFaceFXCharacter::StopAudio()
{
	if(!IsPlayingOrPausedAudio())
	{
		//nothing playing right now
		return true;
	}

	CurrentAudioProgress = 0.F;
	AudioPlaybackState = EPlaybackState::Stopped;

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->SetSound(nullptr);
		AudioComp->Stop();
		return true;
	}
	
	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::StopAudio. Stopping audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool UFaceFXCharacter::ResumeAudio()
{
	if(IsPlayingAudio())
	{
		//already playing audio
		return true;
	}

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioPlaybackState = EPlaybackState::Playing;
		AudioComp->Play(CurrentAudioProgress);
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PauseAudio. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

void UFaceFXCharacter::UpdateTransforms()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXUpdateTransforms);

	const int32 XFormsNum = XForms.Num();
    if(BoneSetHandle && XFormsNum > 0)
    {
	    if(!Check(ffx_calc_frame_bone_xforms(BoneSetHandle, FrameState, &XForms[0], XFormsNum)))
	    {
		    UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::UpdateTransforms. Calculating bone transforms failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		    return;
	    }

	    //fill transform buffer
	    for(int32 i=0; i<XFormsNum; ++i)
	    {
		    const ffx_bone_xform_t& XForm = XForms[i];

		    //the coordinate system of bones is just like the old FaceFX in UE3: native for the animation package you used to create the actor, with the w component of the quaternion negated
		    //All transforms are in parent space.
		    //We also need to bring form FaceFX to UE space by reverting rotation.z and translation.y

		    BoneTransforms[i].SetComponents(
			    //w, x, y, z quaternion rotation
			    FQuat(XForm.rot[1], -XForm.rot[2], XForm.rot[3], XForm.rot[0]),
			    //x, y, z position
			    FVector(XForm.pos[0], -XForm.pos[1], XForm.pos[2]), 
			    //x, y, z scale
			    FVector(XForm.scl[0], XForm.scl[1], XForm.scl[2]));
	    }

	    bIsDirty = false;
    }
}

FString UFaceFXCharacter::GetFaceFXError()
{
	char Msg[128] = {0};
	if (ffx_strerror(ffx_errno(), Msg, 128)) 
	{
		return FString(Msg);
	} 
	else 
	{
		return TEXT("Unable to retrieve additional FaceFX error message.");
	}
}

#if WITH_EDITOR

UFaceFXCharacter::FOnAssetChangedSignature UFaceFXCharacter::OnAssetChanged;

void UFaceFXCharacter::OnFaceFXAssetChanged(UFaceFXAsset* Asset)
{
	UFaceFXAnim* AnimAsset = Cast<UFaceFXAnim>(Asset);
	if(Asset && (Asset == FaceFXActor || (AnimAsset && CurrentAnim == AnimAsset->GetId())))
	{
		if(UFaceFXActor* ActorAsset = Cast<UFaceFXActor>(Asset))
		{
			//actor asset changed -> reload whole actor
			Load(ActorAsset, bDisabledMorphTargets);
		}
		else
		{
			//currently playing actor/anim asset changed -> stop to prevent out of sync playback
			Stop();
		}
	}
}
#endif //WITH_EDITOR
