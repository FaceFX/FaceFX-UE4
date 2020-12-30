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

#include "FaceFXAudioImplDefault.h"
#include "FaceFX.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"

void FFaceFXAudioDefault::Prepare(const UFaceFXAnim* Animation)
{
	check(Animation);

	CurrentAnimSound = bIsAutoPlaySound ? Animation->GetAudio() : nullptr;

	if (bIsAutoPlaySound && !CurrentAnimSound.IsValid() && CurrentAnimSound.ToSoftObjectPath().IsValid())
	{
		//asset not loaded yet -> async load to have it (hopefully) ready when the FaceFX runtime audio start event triggers
		TArray<FSoftObjectPath> StreamingRequests;
		StreamingRequests.Add(CurrentAnimSound.ToSoftObjectPath());
		FaceFX::GetStreamer().RequestAsyncLoad(StreamingRequests, FStreamableDelegate());
	}

	PlaybackState = EPlaybackState::Stopped;
}

bool FFaceFXAudioDefault::Play(float Position, UActorComponent** OutAudioComp)
{
	UFaceFXCharacter* Character = GetOwner();
	check(Character);

	if (bIsAutoPlaySound && CurrentAnimSound.ToSoftObjectPath().IsValid())
	{
		UAudioComponent* AudioComp = GetAudioComponent();
		if (!AudioComp)
		{
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioDefault::Play. Playing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s. Asset: %s")
				, *GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToSoftObjectPath().ToString());
			return false;
		}

		USoundWave* Sound = CurrentAnimSound.Get();
		if (!Sound)
		{
			//sound not loaded (yet) -> load sync now. Here we could also use a delayed audio playback system
			Sound = Cast<USoundWave>(StaticLoadObject(USoundWave::StaticClass(), Character, *CurrentAnimSound.ToSoftObjectPath().ToString()));
		}

		if (Sound)
		{
#if WITH_EDITOR
			UWorld* World = AudioComp->GetWorld();
			if (GIsEditor && World != nullptr && !World->IsPlayInEditor())
            {
                AudioComp->bIsUISound = true;
                AudioComp->bIsPreviewSound = true;
            }
#else
            AudioComp->bIsUISound = false;
#endif // WITH_EDITOR

			CurrentProgress = FMath::Clamp(Position, 0.F, Sound->GetDuration());
			AudioComp->SetSound(Sound);
			AudioComp->SetPaused(false);
			AudioComp->Play(CurrentProgress);

			if (OutAudioComp)
			{
				*OutAudioComp = AudioComp;
			}
			PlaybackState = AudioComp->IsPlaying() ? EPlaybackState::Playing : EPlaybackState::Stopped;

			return IsPlaying();
		}
		else
		{
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioDefault::Play. Playing audio failed. Audio asset failed to load. Actor: %s. Asset: %s"),
				*GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToSoftObjectPath().ToString());
		}
	}

	//no audio set or not to start
	return true;
}


bool FFaceFXAudioDefault::Pause(bool fadeOut)
{
	if (!IsPlaying())
	{
		//nothing playing right now
		return true;
	}

	PlaybackState = EPlaybackState::Paused;

	if (UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->Stop();
		AudioComp->Play(CurrentProgress);

		if (fadeOut)
		{
			//fade out instead of direct stopping to support very short playback durations when play/pausing in one tick (i.e. for scrubbing)
			AudioComp->FadeOut(0.050f, 0.f);
		}
		else
		{
			AudioComp->SetPaused(true);
		}
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioDefault::Pause. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool FFaceFXAudioDefault::Stop(bool enforceAudioCompStop)
{
	if (!IsPlayingOrPaused() && !enforceAudioCompStop)
	{
		//nothing playing right now
		return true;
	}

	CurrentProgress = 0.F;
	PlaybackState = EPlaybackState::Stopped;

	if (UAudioComponent* AudioComp = GetAudioComponent())
	{
		AudioComp->SetSound(nullptr);
		AudioComp->Stop();
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioDefault::Stop. Stopping audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool FFaceFXAudioDefault::Resume()
{
	if (IsPlaying())
	{
		//already playing audio
		return true;
	}

	if (!IsPaused())
	{
		//not in paused state
		return false;
	}

	PlaybackState = EPlaybackState::Playing;

	if (UAudioComponent* AudioComp = GetAudioComponent())
	{
		if (AudioComp->bIsPaused)
		{
			//audio was paused
			AudioComp->SetPaused(false);
		}
		else
		{
			//audio is fading out or was stopped already
			AudioComp->Play(CurrentProgress);
		}
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioDefault::Resume. Resuming audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAudioComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}


bool FFaceFXAudioDefault::SetAudioComponent(UActorComponent* Component)
{
	if (UAudioComponent* ComponentCast = Cast<UAudioComponent>(Component))
	{
		AudioComponent = ComponentCast;
		return true;
	}
	return false;
}

UAudioComponent* FFaceFXAudioDefault::GetAudioComponent() const
{
	if (UAudioComponent* AudioComponentPtr = AudioComponent.Get())
	{
		return AudioComponentPtr;
	}
	
	AActor* OwningActor = GetOwningActor();
	return OwningActor ? OwningActor->FindComponentByClass<UAudioComponent>() : nullptr;
}
