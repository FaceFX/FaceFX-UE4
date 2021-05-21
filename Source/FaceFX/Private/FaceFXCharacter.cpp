/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2021 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXCharacter.h"
#include "FaceFX.h"
#include "FaceFXAllocator.h"
#include "FaceFXActor.h"
#include "FaceFXBlueprintLibrary.h"
#include "Audio/FaceFXAudio.h"
#include "GameFramework/Actor.h"
#include "Animation/FaceFXComponent.h"
#include "Engine/StreamableManager.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(TEXT("Tick Character"), STAT_FaceFXTick, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Update Transforms"), STAT_FaceFXUpdateTransforms, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Load Assets"), STAT_FaceFXLoad, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Play"), STAT_FaceFXPlay, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Broadcast Audio Events"), STAT_FaceFXAudioEvents, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Broadcast Anim Events"), STAT_FaceFXAnimEvents, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Process Morph Targets"), STAT_FaceFXProcessMorphTargets, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Process Material Parameters"), STAT_FaceFXProcessMaterialParameters, STATGROUP_FACEFX);

namespace
{
	EFaceFXBlendMode GetBlendMode(const UFaceFXActor* Dataset)
	{
		check(Dataset);
		EFaceFXBlendMode BlendMode = EFaceFXBlendMode::Replace;
		switch (Dataset->GetBlendMode())
		{
		case EFaceFXActorBlendMode::Global: BlendMode = UFaceFXConfig::Get().GetDefaultBlendMode(); break;
		case EFaceFXActorBlendMode::Additive: BlendMode = EFaceFXBlendMode::Additive; break;
		default: break;
		}

		return BlendMode;
	}

	FxBoneSetFlags GetBoneSetCreationFlags(EFaceFXBlendMode BlendMode, bool IsCompensateForForceFrontXAxis)
	{
		FxBoneSetFlags BoneSetCreationFlags = BlendMode == EFaceFXBlendMode::Additive ? FX_BONESET_OFFSET_XFORMS_BIT : FX_BONESET_FULL_XFORMS;

		if (IsCompensateForForceFrontXAxis)
		{
			BoneSetCreationFlags |= 0x80000000;
		}

		return BoneSetCreationFlags;
	}
}

UFaceFXCharacter::FOnFaceFXCharacterPlayAssetIncompatibleSignature UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible;

UFaceFXCharacter::UFaceFXCharacter(const class FObjectInitializer& PCIP) : Super(PCIP),
	Actor(FX_INVALID_ACTOR),
	FrameState(FX_INVALID_FRAMESTATE),
	BoneSet(FX_INVALID_BONESET),
	CurrentAnimation(FX_INVALID_ANIMATION),
	CurrentTime(0.f),
	CurrentAnimProgress(0.f),
	CurrentAnimDuration(0.f),
	AnimPlaybackState(EPlaybackState::Stopped),
	bIsDirty(true),
	bIsLooping(false),
	bCanPlay(true),
	bCompensatedForForceFrontXAxis(false),
	bDisabledMorphTargets(false),
	bDisabledMaterialParameters(false)
	,bIgnoreEvents(false)
#if WITH_EDITOR
	,LastFrameNumber(0)
#endif
{
	if (!IsTemplate())
	{
#if WITH_EDITOR
		OnFaceFXAnimChangedHandle = UFaceFXCharacter::OnAssetChanged.AddUObject(this, &UFaceFXCharacter::OnFaceFXAssetChanged);
#endif

		//create own audio player
		AudioPlayer = FFaceFXAudio::Create(this);
		check(AudioPlayer.IsValid());
	}
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

bool UFaceFXCharacter::TickUntil(float Duration, bool& OutAudioStarted, bool IgnoreEvents)
{
	if (!bCanPlay || Duration < 0.f)
	{
		return false;
	}

	CurrentTime = Duration;
	CurrentAnimProgress = 0.F;

	//The steps to perform
	static const float TickSteps = 0.1f;

	const bool IgnoreEventsPrev = bIgnoreEvents;
	bIgnoreEvents = IgnoreEvents;

	FxResult ProcessZeroResult = fxActorProcessFrame(Actor, FrameState, 0.f);
	const bool bIsAudioStartedAtZero = IsAudioStarted();
	FxResult Result = fxActorProcessFrame(Actor, FrameState, CurrentTime);

	bIgnoreEvents = IgnoreEventsPrev;

	if (!FX_SUCCEEDED(ProcessZeroResult) || !FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::TickUntil. FaceFX call <fxActorProcessFrame> failed. (zero: %s) %s. Asset: %s"),
		       *FaceFX::GetFaceFXResultString(ProcessZeroResult), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	Result = fxFrameStateGetTrackValues(FrameState, &TrackValues[0], (size_t)TrackValues.Num());

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::TickUntil. FaceFX call <fxFrameStateGetTrackValues> failed. (zero: %s) %s. Asset: %s"),
		       *FaceFX::GetFaceFXResultString(ProcessZeroResult), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	OutAudioStarted = bIsAudioStartedAtZero || IsAudioStarted();
	CurrentAnimProgress = CurrentTime;
	bIsDirty = true;

	ProcessMorphTargets();
	ProcessMaterialParameters();

	return true;
}

AActor* UFaceFXCharacter::GetOwningActor() const
{
	const UFaceFXComponent* OwnerComp = Cast<UFaceFXComponent>(GetOuter());
	return OwnerComp ? OwnerComp->GetOwner() : nullptr;
}

void UFaceFXCharacter::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXTick);

	const bool IsNonZeroTick = DeltaTime > 0.F;
	checkf(!IsNonZeroTick || bCanPlay, TEXT("Internal Error: FaceFX character is not allowed to tick."));

#if WITH_EDITOR
	if (LastFrameNumber == GFrameNumber)
	{
		//tick was called twice within the same frame -> happens when running in PIE with dedicated server active or multiple instances
		return;
	}
	LastFrameNumber = GFrameNumber;
#endif

	//progress in time
	CurrentTime += DeltaTime;
	CurrentAnimProgress += DeltaTime;

	//tick the audio player to update its progression
	AudioPlayer->Tick(DeltaTime);

	FxResult Result = fxActorProcessFrame(Actor, FrameState, CurrentTime);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Tick. FaceFX call <fxActorProcessFrame> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return;
	}

	FxChannelFlags ChannelFlags[FACEFX_CHANNELS];

	Result = fxFrameStateGetChannelFlags(FrameState, ChannelFlags, FACEFX_CHANNELS);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Tick. FaceFX call <fxFrameStateGetChannelFlags> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return;
	}

	const FxChannelFlags EventStoppedCurrentAnimationFlags = FX_CHANNEL_ACTIVE_BIT | FX_CHANNEL_EVENT_FIRED_BIT | FX_CHANNEL_FINISHED_BIT;

	if (EventStoppedCurrentAnimationFlags == ChannelFlags[0])
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Tick(). The FaceFX event handler stopped the currently playing animation."));
		return;
	}

	Result = fxFrameStateGetTrackValues(FrameState, &TrackValues[0], (size_t)TrackValues.Num());

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Tick. FaceFX call <fxFrameStateGetTrackValues> failed. (zero: %s) %s. Asset: %s"),
		       *FaceFX::GetFaceFXResultString(Result), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return;
	}

	if (IsAudioStarted())
	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXAudioEvents);
		UActorComponent* AudioCompStartedOn = nullptr;
		const bool AudioStarted = AudioPlayer->Play(&AudioCompStartedOn);

		OnPlaybackStartAudio.Broadcast(this, GetCurrentAnimationId(), AudioStarted, AudioCompStartedOn);
	}

	ProcessMorphTargets();
	ProcessMaterialParameters();

	bIsDirty = true;

    const bool bIsLastTick = IsNonZeroTick && CurrentAnimProgress >= CurrentAnimDuration;

	if (bIsLastTick)
	{
		if (IsLooping())
		{
			Restart();
		}
		else
		{
			Stop();
		}
	}
}

bool UFaceFXCharacter::IsTickable() const
{
	return IsPlaying() && bCanPlay;
}

TStatId UFaceFXCharacter::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFaceFXCharacter, STATGROUP_Tickables);
}

#if FACEFX_USEANIMATIONLINKAGE
bool UFaceFXCharacter::GetAnimationBoundsById(const AActor* Actor, const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd)
{
	if (Actor)
	{
		if (const UFaceFXComponent* FaceFXComp = Actor->FindComponentByClass<UFaceFXComponent>())
		{
			if (const FFaceFXEntry* FxEntry = FaceFXComp->GetCharacterEntry())
			{
				if (FxEntry->Character)
				{
					//get via character
					return FxEntry->Character->GetAnimationBoundsById(AnimId, OutStart, OutEnd);
				}
				else if (FxEntry->Asset.IsValid())
				{
					//fetch from FaceFX actor
					if (const UFaceFXActor* FxActor = TSoftObjectPtr<UFaceFXActor>(FxEntry->Asset).LoadSynchronous())
					{
						return UFaceFXCharacter::GetAnimationBoundsById(FxActor, AnimId, OutStart, OutEnd);
					}
				}
			}
		}
	}
	return false;
}

bool UFaceFXCharacter::GetAnimationBoundsById(const UFaceFXActor* FaceFXActor, const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd)
{
	if (!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBoundsById. FaceFX asset not loaded."));
		return false;
	}

	if (const UFaceFXAnim* Anim = FaceFXActor->GetAnimation(AnimId))
	{
		return FaceFX::GetAnimationBounds(Anim, OutStart, OutEnd);
	}
	else
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBoundsById. Animation does not exist. Group=%s, Anim=%s. Asset: %s"), *AnimId.Group.GetPlainNameString(), *AnimId.Name.GetPlainNameString(), *GetNameSafe(FaceFXActor));
	}

	return false;
}

bool UFaceFXCharacter::GetAllLinkedAnimationIds(TArray<FFaceFXAnimId>& OutAnimIds) const
{
	if (!FaceFXActor)
	{
		return false;
	}

	FaceFXActor->GetAnimationIds(OutAnimIds);
	return true;
}

#endif //FACEFX_USEANIMATIONLINKAGE

bool UFaceFXCharacter::GetAnimationBounds(float& OutStart, float& OutEnd) const
{
	if (!IsPlaying() || !CurrentAnimation)
	{
		return false;
	}

	FxResult Result = fxAnimationGetBounds(CurrentAnimation, &OutStart, &OutEnd);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::GetAnimationBounds. FaceFX call <fxAnimationGetBounds> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	return true;
}

#if FACEFX_USEANIMATIONLINKAGE

bool UFaceFXCharacter::Play(const FFaceFXAnimId& AnimId, bool Loop)
{
	if (!AnimId.IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. Invalid animation name/group. Group=%s, Anim=%s. Asset: %s"), *AnimId.Group.GetPlainNameString(), *AnimId.Name.GetPlainNameString(), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX asset not loaded."));
		return false;
	}

	if (const UFaceFXAnim* Anim = FaceFXActor->GetAnimation(AnimId))
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

	if (!Animation)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX animation asset missing."));
		return false;
	}

	if (!Animation->IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX animation asset is in an invalid state. Please reimport that asset. Asset: %s"), *GetNameSafe(Animation));
		return false;
	}

	if (!bCanPlay)
	{
		return false;
	}

	if (!FaceFXActor)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX asset not loaded."));
		return false;
	}

	if (!IsLoaded())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (IsPlayingOrPaused())
	{
		if (GetCurrentAnimationId() != Animation->GetId())
		{
			//warn only about any animation that is not getting restarted
			UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Play. Stopping currently playing/paused animation %s to play animation %s Actor: %s."), *GetNameSafe(CurrentAnim), *GetNameSafe(Animation), *GetNameSafe(FaceFXActor));
		}
		Stop();
	}

	if (GetCurrentAnimationId() != Animation->GetId())
	{
		//animation changed -> create new handle

		//check if we actually can play this animation
		FxAnimation NewAnimation = FaceFX::LoadAnimation(Animation->GetData());

		if (!IsCanPlay(NewAnimation))
		{
			UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Play. Animation is not compatible with FaceFX actor. Actor: %s. Animation: %s"), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
			OnFaceFXCharacterPlayAssetIncompatible.Broadcast(this, Animation);

			//destroy the animation that can't be played so it isn't leaked.
			FxResult Result = fxAnimationDestroy(&NewAnimation, nullptr, nullptr);

			if (!FX_SUCCEEDED(Result))
			{
				UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call <fxAnimationDestroy> failed. %s. Actor: %s Animation: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
			}

			return false;
		}

		//unload previous animation
		UnloadCurrentAnim();

		CurrentAnimation = NewAnimation;

		AudioPlayer->Prepare(Animation);
	}

	//get anim bounds
	float AnimStart = 0.f;
	float AnimEnd = 0.f;

	FxResult Result = fxAnimationGetBounds(CurrentAnimation, &AnimStart, &AnimEnd);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call <fxAnimationGetBounds> failed. %s. Actor: %s. Animation: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	//sanity check
	CurrentAnimDuration = AnimEnd - AnimStart;

	if (CurrentAnimDuration <= 0.f)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. Invalid animation length of %.3f seconds. Actor: %s. Animation: %s"), CurrentAnimDuration, *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	Result = fxActorPlayAnimation(Actor, CurrentAnimation, nullptr);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Play. FaceFX call <fxActorPlayAnimation> failed. %s. Actor: %s. Animation: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		return false;
	}

	//reset timers and states
	CurrentAnimProgress = 0.f;
	CurrentAnim = Animation;
	CurrentAnimStart = AnimStart;
	AnimPlaybackState = EPlaybackState::Playing;
	bIsLooping = Loop;

	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXAudioEvents);
		OnPlaybackStarted.Broadcast(this, GetCurrentAnimationId());
	}

	return true;
}

bool UFaceFXCharacter::Resume()
{
	if (!bCanPlay)
	{
		return false;
	}

	if (!IsLoaded())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Resume. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (IsPlaying())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::Resume. Animation still playing. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	FxResult Result = fxActorResumeAnimation(Actor, CurrentTime);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Resume. FaceFX call <fxActorResumeAnimation> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	AnimPlaybackState = EPlaybackState::Playing;
	AudioPlayer->Resume();

	return true;
}

bool UFaceFXCharacter::Pause(bool fadeOut)
{
	if (!IsLoaded())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Pause. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (IsPlaying())
	{
		FxResult Result = fxActorPauseAnimation(Actor, CurrentTime);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Pause. FaceFX call <fxActorPauseAnimation> failed. %s Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
			return false;
		}
	}

	AnimPlaybackState = EPlaybackState::Paused;
	AudioPlayer->Pause(fadeOut);

	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXAudioEvents);
		OnPlaybackPaused.Broadcast(this, GetCurrentAnimationId());
	}

	return true;
}

bool UFaceFXCharacter::Stop(bool enforceStop)
{
	if (!IsLoaded() && !enforceStop)
	{
		//not loaded yet
		return false;
	}

	const bool WasPlayingOrPaused = IsPlayingOrPaused();

	if (WasPlayingOrPaused)
	{
		FxResult Result = fxActorStopAnimation(Actor, FX_CHANNEL_ANY);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Stop. FaceFX call <fxActorStopAnimation> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
			return false;
		}
	}

	const FFaceFXAnimId StoppedAnimId = GetCurrentAnimationId();

	//reset timer and audio query indicator
	CurrentAnimProgress = .0F;
	CurrentAnim = nullptr;
	AnimPlaybackState = EPlaybackState::Stopped;
	AudioPlayer->Stop(enforceStop);

	UnloadCurrentAnim();

	ResetMaterialParametersToDefaults();

	if (WasPlayingOrPaused)
	{
		SCOPE_CYCLE_COUNTER(STAT_FaceFXAudioEvents);
		OnPlaybackStopped.Broadcast(this, StoppedAnimId);
	}

	return true;
}

bool UFaceFXCharacter::JumpTo(float Position)
{
	if (Position < 0.F || (!IsLooping() && Position > CurrentAnimDuration))
	{
		return false;
	}

	if (!bCanPlay)
	{
		return false;
	}

	if (!IsLoaded())
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::JumpTo. FaceFX character not loaded. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	if (!CurrentAnimation)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. Current animation is invalid. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	//explicitly stop and start the playback again
	FxResult Result = fxActorStopAnimation(Actor, FX_CHANNEL_ANY);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. FaceFX call <fxActorStopAnimation> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	AnimPlaybackState = EPlaybackState::Stopped;

	//play again
	Result = fxActorPlayAnimation(Actor, CurrentAnimation, nullptr);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::JumpTo. FaceFX call <fxActorPlayAnimation> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		return false;
	}

	AnimPlaybackState = EPlaybackState::Playing;

	if (Position > CurrentAnimDuration)
	{
		//cap the position to the animation range
		Position = FMath::Fmod(Position, CurrentAnimDuration);
	}

	bool IsAudioStarted;
	if (TickUntil(Position, IsAudioStarted) && IsAudioStarted)
	{
		const float AudioPosition = Position + CurrentAnimStart;
		checkf(AudioPosition >= 0.F, TEXT("Invalid audio playback range."));
		return AudioPlayer->Play(AudioPosition);
	}

	AudioPlayer->Stop();
	return true;
}

void UFaceFXCharacter::Reset()
{
	//Stop any playing animation before destroying the handles
	Stop();

	//free the facefx handles
	UnloadCurrentAnim();

	if (Actor)
	{
		FxResult Result = fxActorDestroy(&Actor, nullptr, nullptr);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Reset. FaceFX call <fxActorDestroy> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		}
	}

	if (FrameState)
	{
		FxResult Result = fxFrameStateDestroy(&FrameState);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Reset. FaceFX call <fxFrameStateDestroy> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		}
	}

	if (BoneSet)
	{
		FxResult Result = fxBoneSetDestroy(&BoneSet, nullptr, nullptr);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Reset. FaceFX call <fxBoneSetDestroy> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		}
	}

	//reset arrays
	TrackValues.Empty();
	FaceFXBoneTransforms.Empty();
	BoneTransforms.Empty();
	BoneIds.Empty();

	ResetMorphTargets();
	ResetMaterialParameters();

	FaceFXActor = nullptr;

	bIsDirty = true;
}

bool UFaceFXCharacter::IsPlaying(const UFaceFXAnim* Animation) const
{
	return Animation && IsPlaying(Animation->GetId());
}

bool UFaceFXCharacter::IsPlayingOrPaused(const UFaceFXAnim* Animation) const
{
	return Animation && IsPlayingOrPaused(Animation->GetId());
}

void UFaceFXCharacter::OnFaceFXEvent(const FxEventFiringContext* Context, const char* Payload)
{
	UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::OnFaceFXEventReceived. FaceFX event received: %s."), ANSI_TO_TCHAR(Payload));

	if (UFaceFXCharacter* Character = static_cast<UFaceFXCharacter*>(Context->pUserData))
	{
		if (!Character->bIgnoreEvents && Context->actor == Character->Actor && Context->animation == Character->CurrentAnimation)
		{
			SCOPE_CYCLE_COUNTER(STAT_FaceFXAnimEvents);
			Character->OnAnimationEvent.Broadcast(Character, Character->GetCurrentAnimationId(), (int)Context->channelIndex, Context->channelTime, Context->eventTime, Payload);
		}
	}
}

bool UFaceFXCharacter::Load(const UFaceFXActor* Dataset, bool IsCompensateForForceFrontXAxis, bool IsDisabledMorphTargets, bool IsDisableMaterialParameters)
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXLoad);

	if (!Dataset)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Missing FaceFXActor asset."));
		return false;
	}

	if (!Dataset->IsValid())
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Invalid FaceFXActor asset. Please reimport that asset. Asset: %s"), *GetNameSafe(FaceFXActor));
		return false;
	}

	Reset();

	FaceFXActor = Dataset;
	BlendMode = ::GetBlendMode(Dataset);

	const FFaceFXActorData& ActorData = FaceFXActor->GetData();

	FxAllocationCallbacks Allocator = FFaceFXAllocator::CreateAllocator();

	//only create the bone set handle if there is bone set data
	if (ActorData.BonesRawData.Num() > 0)
	{
		const FxBoneSetFlags BoneSetCreationFlags = ::GetBoneSetCreationFlags(BlendMode, IsCompensateForForceFrontXAxis);

		FxResult Result = fxBoneSetCreate(&ActorData.BonesRawData[0], ActorData.BonesRawData.Num(), FX_DATA_VALIDATION_ON, BoneSetCreationFlags, &BoneSet, &Allocator);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX bone set. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
			Reset();
			return false;
		}

		if (Result == FX_WARNING_LEGACY_DATA_FORMAT)
		{
			UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::Load. Loaded a legacy data format. Please recompile the content with the latest FaceFX Runtime compiler. Asset: %s"), *GetNameSafe(FaceFXActor));
		}
	}

	//make sure there is actor data
	if (ActorData.ActorRawData.Num() == 0)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. No FaceFX actor data present. Asset: %s"), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	constexpr size_t ChannelCount = FACEFX_CHANNELS;

	FxEventCallbacks EventHandler;
	EventHandler.pfnEventFired = UFaceFXCharacter::OnFaceFXEvent;
	EventHandler.pUserData = this;

	FxResult Result = fxActorCreateWithEventHandler(&ActorData.ActorRawData[0], ActorData.ActorRawData.Num(), FX_DATA_VALIDATION_ON, ChannelCount, &Actor, &EventHandler, &Allocator);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX actor handle. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	if (Result == FX_WARNING_LEGACY_DATA_FORMAT)
	{
		UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::Load. Loaded a legacy data format. Please recompile the content with the latest FaceFX Runtime compiler. Asset: %s"), *GetNameSafe(FaceFXActor));
	}

	size_t TrackCount = 0;

	Result = fxActorGetTracks(Actor, nullptr, &TrackCount);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to retrieve FaceFX tracks. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	TrackValues.AddUninitialized(TrackCount);

	TArray<uint64_t> TrackIds;
	TrackIds.AddUninitialized(TrackCount);

	Result = fxActorGetTracks(Actor, &TrackIds[0], &TrackCount);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to retrieve FaceFX tracks. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	Result = fxFrameStateCreate(Actor, &FrameState, &Allocator);

	if (!FX_SUCCEEDED(Result))
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to create FaceFX frame state. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		Reset();
		return false;
	}

	if (BoneSet)
	{
		size_t XFormCount = 0;

		Result = fxBoneSetGetBones(BoneSet, nullptr, &XFormCount);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to retrieve FaceFX bone count. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
			Reset();
			return false;
		}

		if (XFormCount > 0)
		{
			//prepare buffer
			FaceFXBoneTransforms.AddUninitialized(XFormCount);

			//retrieve bone names
			BoneIds.AddUninitialized(XFormCount);

			Result = fxBoneSetGetBones(BoneSet, BoneIds.GetData(), &XFormCount);

			if (!FX_SUCCEEDED(Result))
			{
				UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::Load. Unable to retrieve FaceFX bones. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
				Reset();
				return false;
			}

			//prepare transform buffer
			BoneTransforms.AddZeroed(XFormCount);

			//match bone ids with names from the .ffxids assets
			for (const uint64_t& BoneIdHash : BoneIds)
			{
				if (const FFaceFXIdData* BoneId = ActorData.Ids.FindByKey(BoneIdHash))
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

	bCompensatedForForceFrontXAxis = IsCompensateForForceFrontXAxis;
	bDisabledMorphTargets = IsDisabledMorphTargets;
	bDisabledMaterialParameters = IsDisableMaterialParameters;

	ResetMorphTargets();
	ResetMaterialParameters();
	ResetMaterialParametersToDefaults();

	//SetupMaterialParameters after SetupMorphTargets as we ignore the morph target tracks as material parameters
	if ( (!bDisabledMorphTargets && !SetupMorphTargets(Dataset, TrackIds)) ||
		(!IsDisableMaterialParameters && !SetupMaterialParameters(Dataset, TrackIds, MorphTargetNames)) )
	{
		Reset();
		return false;
	}

	return true;
}

void UFaceFXCharacter::ProcessMorphTargets()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXProcessMorphTargets);

	const int32 MorphTargetsToProcess = MorphTargetNames.Num();

	if (MorphTargetsToProcess == 0)
	{
		return;
	}

	if (USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent())
	{
		for (int32 Idx = 0; Idx < MorphTargetsToProcess; ++Idx)
		{
			SkelMeshComp->SetMorphTarget(MorphTargetNames[Idx], TrackValues[MorphTargetIndices[Idx]]);
		}
	}
	else
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::ProcessMorphTargets. Unable to find owners skel mesh component. Actor: %s. Asset: %s"), *GetNameSafe(GetOwningActor()), *GetNameSafe(FaceFXActor));
	}
}

bool UFaceFXCharacter::SetupMorphTargets(const UFaceFXActor* Dataset, const TArray<uint64_t>& TrackIds)
{
	check(IsLoaded());
	check(Dataset);

	USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent();

	if (!SkelMeshComp)
	{
		return true;
	}

	const int32 NumMorphTargets = SkelMeshComp->SkeletalMesh ? SkelMeshComp->SkeletalMesh->MorphTargetIndexMap.Num() : 0;

	if (NumMorphTargets == 0)
	{
		return true;
	}

	MorphTargetNames.Reserve(NumMorphTargets);
	MorphTargetIndices.Reserve(NumMorphTargets);

	int32 NumTracks = TrackIds.Num();

	for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
	{
		const uint64_t TrackId = TrackIds[TrackIndex];

		const FFaceFXIdData* AssetIdData = Dataset->GetData().Ids.FindByKey(TrackId);

		if (!AssetIdData)
		{
			continue;
		}

		if (SkelMeshComp->SkeletalMesh->MorphTargetIndexMap.Contains(AssetIdData->Name))
		{
			MorphTargetNames.Add(AssetIdData->Name);
			MorphTargetIndices.Add((size_t)TrackIndex);

			UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::SetupMorphTargets. driving morph target named %s (Asset: %s)"), *AssetIdData->Name.ToString(), *GetNameSafe(FaceFXActor));
		}
	}

	return true;
}

bool UFaceFXCharacter::SetupMaterialParameters(const UFaceFXActor* Dataset, const TArray<uint64_t>& TrackIds, const TArray<FName>& IgnoredTracks)
{
	check(IsLoaded());
	check(Dataset);

	USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent();

	if (!SkelMeshComp)
	{
		return false;
	}

	const int32 NumMaterials = SkelMeshComp->GetNumMaterials();

	if (NumMaterials == 0)
	{
		return false;
	}

	MaterialParameterNames.Reserve(NumMaterials);
	MaterialParameterIndices.Reserve(NumMaterials);

	int32 NumTracks = TrackIds.Num();

	for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
	{
		const uint64_t TrackId = TrackIds[TrackIndex];

		const FFaceFXIdData* AssetIdData = Dataset->GetData().Ids.FindByKey(TrackId);

		if (!AssetIdData)
		{
			continue;
		}

		if (IgnoredTracks.Contains(AssetIdData->Name))
		{
			continue;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			if (UMaterialInterface* Material = SkelMeshComp->GetMaterial(MaterialIndex))
			{
				float ParamterValue;

				if (Material->GetScalarParameterValue(AssetIdData->Name, ParamterValue))
				{
					MaterialParameterNames.Add(AssetIdData->Name);
					MaterialParameterIndices.Add((size_t)TrackIndex);

					UE_LOG(LogFaceFX, Verbose, TEXT("UFaceFXCharacter::SetupMaterialParameters. driving material parameter named %s (Asset: %s)"), *AssetIdData->Name.ToString(), *GetNameSafe(FaceFXActor));

					break;
				}
			}
		}
	}

	return true;
}

void UFaceFXCharacter::ProcessMaterialParameters()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXProcessMaterialParameters);

	const int32 MaterialParametersToProcess = MaterialParameterNames.Num();

	if (MaterialParametersToProcess == 0)
	{
		return;
	}

	if (USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent())
	{
		for (int32 Idx = 0; Idx < MaterialParametersToProcess; ++Idx)
		{
			SkelMeshComp->SetScalarParameterValueOnMaterials(MaterialParameterNames[Idx], TrackValues[MaterialParameterIndices[Idx]]);
		}
	}
	else
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::ProcessMaterialParameters. Unable to find owners skel mesh component. Actor: %s. Asset: %s"), *GetNameSafe(GetOwningActor()), *GetNameSafe(FaceFXActor));
	}
}

void UFaceFXCharacter::ResetMaterialParametersToDefaults()
{
	if (USkeletalMeshComponent* SkelMeshComp = GetOwningSkelMeshComponent())
	{
		for (int32 Idx = 0; Idx < MaterialParameterNames.Num(); ++Idx)
		{
			const FName& ParameterName = MaterialParameterNames[Idx];

			const float DefaultValue = SkelMeshComp->GetScalarParameterDefaultValue(ParameterName);

			SkelMeshComp->SetScalarParameterValueOnMaterials(ParameterName, DefaultValue);
		}
	}
}

bool UFaceFXCharacter::IsCanPlay(const UFaceFXAnim* Animation) const
{
	bool CanPlay = false;

	if (Animation)
	{
		FxAnimation NewAnimation = FaceFX::LoadAnimation(Animation->GetData());

		CanPlay = IsCanPlay(NewAnimation);

		//destroy the animation so it isn't leaked.
		FxResult Result = fxAnimationDestroy(&NewAnimation, nullptr, nullptr);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::IsCanPlay. FaceFX call <fxAnimationDestroy> failed. %s. Actor: %s Animation: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor), *GetNameSafe(Animation));
		}
	}

	return Animation && CanPlay;
}

bool UFaceFXCharacter::IsCanPlay(FxAnimation Animation) const
{
	return Actor && Animation && (FX_SUCCEEDED(fxActorCheckCompatibilityWithAnimation(Actor, Animation)));
}

bool UFaceFXCharacter::IsPlayingAudio() const
{
	check(AudioPlayer.IsValid());
	return AudioPlayer->IsPlaying();
}

bool UFaceFXCharacter::IsPlayingOrPausedAudio() const
{
	check(AudioPlayer.IsValid());
	return AudioPlayer->IsPlayingOrPaused();
}

bool UFaceFXCharacter::IsAutoPlaySound() const
{
	check(AudioPlayer.IsValid());
	return AudioPlayer->IsAutoPlaySound();
}

void UFaceFXCharacter::SetAutoPlaySound(bool isAutoPlaySound)
{
	check(AudioPlayer.IsValid());
	AudioPlayer->SetAutoPlaySound(isAutoPlaySound);
}

void UFaceFXCharacter::SetAudioComponent(UActorComponent* Component)
{
	check(AudioPlayer.IsValid());
	AudioPlayer->SetAudioComponent(Component);
}

bool UFaceFXCharacter::IsAudioStarted()
{
	FxChannelFlags ChannelFlags[FACEFX_CHANNELS];

	FxResult Result = fxFrameStateGetChannelFlags(FrameState, ChannelFlags, FACEFX_CHANNELS);

	if (FX_SUCCEEDED(Result))
	{
		return (ChannelFlags[0] & FX_CHANNEL_START_AUDIO_BIT) != 0;
	}
	else
	{
		UE_LOG(LogFaceFX, Warning, TEXT("UFaceFXCharacter::IsAudioStarted. Retrieving channel flags for StartAudio event failed. %s"), *FaceFX::GetFaceFXResultString(Result));
	}

	return false;
}

int32 UFaceFXCharacter::GetBoneNameTransformIndex(const FName& Name) const
{
	if (const FFaceFXIdData* BoneId = FaceFXActor ? FaceFXActor->GetData().Ids.FindByKey(Name) : nullptr)
	{
		//get index of the FaceFX bone
		int32 FaceFXBoneIdx;
		if (BoneIds.Find(BoneId->Id, FaceFXBoneIdx))
		{
			return FaceFXBoneIdx;
		}
	}
	return INDEX_NONE;
}

void UFaceFXCharacter::UnloadCurrentAnim()
{
	if (CurrentAnimation)
	{
		FxResult Result = fxAnimationDestroy(&CurrentAnimation, nullptr, nullptr);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::UnloadCurrentAnim. FaceFX call <fxAnimationDestroy> failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
		}
	}

	CurrentAnim = nullptr;
}

FFaceFXAnimId UFaceFXCharacter::GetCurrentAnimationId() const
{
	return CurrentAnim ? CurrentAnim->GetId() : FFaceFXAnimId();
}

USkeletalMeshComponent* UFaceFXCharacter::GetOwningSkelMeshComponent() const
{
	const UFaceFXComponent* FaceFXComp = GetOwningFaceFXComponent();
	return FaceFXComp ? FaceFXComp->GetSkelMeshTarget(this) : nullptr;
}

UFaceFXComponent* UFaceFXCharacter::GetOwningFaceFXComponent() const
{
	return IsPendingKill() ? nullptr : Cast<UFaceFXComponent>(GetOuter());
}

void UFaceFXCharacter::UpdateTransforms()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXUpdateTransforms);

	const int32 FaceFXBoneTransformsNum = FaceFXBoneTransforms.Num();
	if (BoneSet && FaceFXBoneTransformsNum > 0)
	{
		FxResult Result = fxFrameStateComputeBoneTransforms(BoneSet, FrameState, &FaceFXBoneTransforms[0], FaceFXBoneTransformsNum);

		if (!FX_SUCCEEDED(Result))
		{
			UE_LOG(LogFaceFX, Error, TEXT("UFaceFXCharacter::UpdateTransforms. Calculating bone transforms failed. %s. Asset: %s"), *FaceFX::GetFaceFXResultString(Result), *GetNameSafe(FaceFXActor));
			return;
		}

		//fill transform buffer
		for (int32 i=0; i<FaceFXBoneTransformsNum; ++i)
		{
			const FxBoneTransform& XForm = FaceFXBoneTransforms[i];

			// Revert rotaiton.z and translation.y to convert from FaceFX to UE4 coordinates.
			BoneTransforms[i].SetComponents(
				FQuat(XForm.rotation.x, -XForm.rotation.y, XForm.rotation.z, XForm.rotation.w),
				FVector(XForm.translation.x, -XForm.translation.y, XForm.translation.z),
				FVector(XForm.scale.x, XForm.scale.y, XForm.scale.z));
		}

		bIsDirty = false;
	}
}


#if WITH_EDITOR

UFaceFXCharacter::FOnAssetChangedSignature UFaceFXCharacter::OnAssetChanged;

void UFaceFXCharacter::OnFaceFXAssetChanged(UFaceFXAsset* Asset)
{
	UFaceFXAnim* AnimAsset = Cast<UFaceFXAnim>(Asset);
	if (Asset && (Asset == FaceFXActor || (AnimAsset && GetCurrentAnimationId() == AnimAsset->GetId())))
	{
		if (UFaceFXActor* ActorAsset = Cast<UFaceFXActor>(Asset))
		{
			//actor asset changed -> reload whole actor
			Load(ActorAsset, bCompensatedForForceFrontXAxis, bDisabledMorphTargets, bDisabledMaterialParameters);
		}
		else
		{
			//currently playing actor/anim asset changed -> stop to prevent out of sync playback
			Stop();
		}
	}
}
#endif //WITH_EDITOR
