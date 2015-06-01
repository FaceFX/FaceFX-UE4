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
#include "Animation/FaceFxComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"

DECLARE_STATS_GROUP(TEXT("FaceFX"),STATGROUP_FACEFX, STATCAT_Advance);
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
	bIsDirty(true),
	bIsPlaying(false),
	bIsPlayingAudio(false),
	bIsLooping(false),
	bCanPlay(true)
#if WITH_EDITOR
	,LastFrameNumber(0)
#endif
{
}

void UFaceFXCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	bCanPlay = false;
	Reset();	
}

void UFaceFXCharacter::Tick(float DeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FaceFXTick);

	checkf(bCanPlay, TEXT("Internal Error: FaceFX character is not allowed to tick."));

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

	if(CurrentAnimProgress >= CurrentAnimDuration)
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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Tick. Update failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return;
	}

	if(OnPlaybackStartAudio.IsBound())
	{
		int channelFlags[FACEFX_CHANNELS];
		if(Check(ffx_read_frame_channel_flags(FrameState, channelFlags, FACEFX_CHANNELS)))
		{
			if(channelFlags[0] & FFX_START_AUDIO)
			{
				{
					SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
					UAudioComponent* AudioCompStartedOn = nullptr;
					const bool AudioStarted = PlayAudio(&AudioCompStartedOn);
					
					OnPlaybackStartAudio.Broadcast(this, CurrentAnim, AudioStarted, AudioCompStartedOn);
					
				}
			}		
		}
		else
		{
			UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Tick. Retrieving channel states for StartAudio event failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		}
	}

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

bool UFaceFXCharacter::GetAnimationBounds(float& OutStart, float& OutEnd) const
{
	if(!IsPlaying() || !CurrentAnimHandle)
	{
		return false;
	}

	//get anim bounds
	if(!Check(ffx_get_anim_bounds(CurrentAnimHandle, &OutStart, &OutEnd)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBounds. FaceFX call failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
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

	return Play(FaceFXActor->GetAnimation(AnimId), Loop);
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

	if(IsPlaying())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Play. Animation already playing. Stopping now. Actor: %s. Animation: %s"), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call failed. %s. Actor: %s. Animation: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call failed. %s. Actor: %s. Animation: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	//reset timers and states
	CurrentAnimProgress = 0.F;
	CurrentAnim = Animation->GetId();
	bIsPlaying = true;
	bIsLooping = Loop;

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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Resume. FaceFX call failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}
	
	bIsPlaying = true;
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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Pause. FaceFX call failed. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	bIsPlaying = false;
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

	if ((IsPlaying() || IsPaused()) && !Check(ffx_stop(ActorHandle)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Stop. FaceFX call failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	const FFaceFXAnimId StoppedAnimId = CurrentAnim;

	//reset timer and audio query indicator
	CurrentAnimProgress = .0F;
	CurrentAnim.Reset();
	bIsPlaying = false;
	StopAudio();

	UnloadCurrentAnim();

	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXEvents);
		OnPlaybackStopped.Broadcast(this, StoppedAnimId);
	}

	return true;
}

bool UFaceFXCharacter::Restart()
{
	if(!bCanPlay)
	{
		return false;
	}

	if(!IsLoaded())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Restart. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if(!CurrentAnimHandle)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Restart. Current animation is invalid. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	//explicitly stop and start the playback again
	if (!Check(ffx_stop(ActorHandle)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Restart. FaceFX call failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	//play again
	if(!Check(ffx_play(ActorHandle, CurrentAnimHandle, nullptr)))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Restart. FaceFX call failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return false;
	}

	//reset timer
	CurrentAnimProgress = 0.F;
	
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
	
	FaceFXActor = nullptr;

	bIsDirty = true;
}

bool UFaceFXCharacter::Load(const UFaceFXActor* Dataset)
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

	//  We chose a global context for this sample.
	ffx_context_t Context = FFaceFXContext::CreateContext();

	if (!Check(ffx_create_bone_set_handle((char*)(&ActorData.BonesRawData[0]), ActorData.BonesRawData.Num(), FFX_RUN_INTEGRITY_CHECK, FFX_USE_FULL_XFORMS, &BoneSetHandle, &Context))) 
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX bone handle. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
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

		{
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

AActor* UFaceFXCharacter::GetOwnningActor() const
{
	const UFaceFXComponent* OwnerComp = Cast<UFaceFXComponent>(GetOuter());
	return OwnerComp ? OwnerComp->GetOwner() : nullptr;
}

UAudioComponent* UFaceFXCharacter::GetAudioComponent() const
{
	AActor* Owner = GetOwnningActor();
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

bool UFaceFXCharacter::PlayAudio(class UAudioComponent** OutAudioComp)
{
	if(bIsAutoPlaySound && CurrentAnimSound.ToStringReference().IsValid())
	{
		UAudioComponent* AudioComp = GetAudioComponent();

		if(!AudioComp)
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PlayAudio. Playing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s. Asset: %s")
				, *GetNameSafe(GetOwnningActor()), *CurrentAnimSound.ToStringReference().ToString());
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
			AudioComp->SetSound(Sound);
			AudioComp->Play();

			if(OutAudioComp)
			{
				*OutAudioComp = AudioComp;
			}
			bIsPlayingAudio = AudioComp->IsPlaying();
			return bIsPlayingAudio;
		}
		else
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PlayAudio. Playing audio failed. Audio asset failed to load. Actor: %s. Asset: %s"), 
				*GetNameSafe(GetOwnningActor()), *CurrentAnimSound.ToStringReference().ToString());
		}
	}

	return false;
}

bool UFaceFXCharacter::PauseAudio()
{
	if(!bIsPlayingAudio)
	{
		//nothing playing right now
		return true;
	}

	bIsPlayingAudio = false;

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->Stop();
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PauseAudio. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwnningActor()));
	return false;
}

bool UFaceFXCharacter::StopAudio()
{
	if(!bIsPlayingAudio)
	{
		//nothing playing right now
		return true;
	}

	CurrentAudioProgress = 0.F;
	bIsPlayingAudio = false;

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->Stop();
		return true;
	}
	
	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::StopAudio. Stopping audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwnningActor()));
	return false;
}

bool UFaceFXCharacter::ResumeAudio()
{
	if(bIsPlayingAudio)
	{
		//already playing audio
		return true;
	}

	if(UAudioComponent* AudioComp = GetAudioComponent())
	{
		bIsPlayingAudio = true;
		AudioComp->Play(CurrentAudioProgress);
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::PauseAudio. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwnningActor()));
	return false;
}

void UFaceFXCharacter::UpdateTransforms()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXUpdateTransforms);
	
	if(XForms.Num() == 0)
	{
		//no buffer created yet
		return;
	}

	if(!Check(ffx_calc_frame_bone_xforms(BoneSetHandle, FrameState, &XForms[0], XForms.Num())))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::UpdateTransforms. Calculating bone transforms failed. %s. Asset: %s"), *GetFaceFXError(), *GetNameSafe(FaceFXActor));
		return;
	}

	//fill transform buffer
	for(int32 i=0; i<XForms.Num(); ++i)
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