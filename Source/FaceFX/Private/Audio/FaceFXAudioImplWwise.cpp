/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2020 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXAudioImplWwise.h"
#include "FaceFX.h"

#if WITH_WWISE

#include "AkComponent.h"
#include "AkAudioEvent.h"

/**
* Loads the AK audio event if not loaded yet
* @param Character The owning character
* @param Asset The asset to load
* @returns The loaded AK event asset
*/
UAkAudioEvent* GetAkEvent(UFaceFXCharacter* Character, const TSoftObjectPtr<UAkAudioEvent>& Asset)
{
	UAkAudioEvent* SoundEvent = Asset.Get();
	if (!SoundEvent)
	{
		//sound not loaded (yet) -> load sync now. Here we could also use a delayed audio playback system
		SoundEvent = Cast<UAkAudioEvent>(StaticLoadObject(UAkAudioEvent::StaticClass(), Character, *Asset.ToSoftObjectPath().ToString()));
	}
	return SoundEvent;
}

void FFaceFXAudioWwise::Prepare(const UFaceFXAnim* Animation)
{
	check(Animation);

	if (bIsAutoPlaySound)
	{
		CurrentAnimSound = Animation->GetAudioAkEvent().ToSoftObjectPath();
		CurrentAnimSoundStop = Animation->GetAudioAkEventStop().ToSoftObjectPath();
		CurrentAnimSoundPause = Animation->GetAudioAkEventPause().ToSoftObjectPath();
		CurrentAnimSoundResume = Animation->GetAudioAkEventResume().ToSoftObjectPath();

		TArray<FSoftObjectPath> StreamingRequests;

		//check if asset are not loaded yet -> async load to have it (hopefully) ready when the FaceFX runtime audio start event triggers
		if (!CurrentAnimSound.IsValid() && CurrentAnimSound.ToSoftObjectPath().IsValid())
		{
			StreamingRequests.Add(CurrentAnimSound.ToSoftObjectPath());
		}
		if (!CurrentAnimSoundStop.IsValid() && CurrentAnimSoundStop.ToSoftObjectPath().IsValid())
		{
			StreamingRequests.Add(CurrentAnimSoundStop.ToSoftObjectPath());
		}
		if (!CurrentAnimSoundPause.IsValid() && CurrentAnimSoundPause.ToSoftObjectPath().IsValid())
		{
			StreamingRequests.Add(CurrentAnimSoundPause.ToSoftObjectPath());
		}
		if (!CurrentAnimSoundResume.IsValid() && CurrentAnimSoundResume.ToSoftObjectPath().IsValid())
		{
			StreamingRequests.Add(CurrentAnimSoundResume.ToSoftObjectPath());
		}

		if (StreamingRequests.Num() > 0)
		{
			FaceFX::GetStreamer().RequestAsyncLoad(StreamingRequests, FStreamableDelegate());
		}
	}
	else
	{
		CurrentAnimSound.Reset();
		CurrentAnimSoundStop.Reset();
		CurrentAnimSoundPause.Reset();
		CurrentAnimSoundResume.Reset();
	}

	PlaybackState = EPlaybackState::Stopped;
}

bool FFaceFXAudioWwise::Play(float Position, UActorComponent** OutAudioComp)
{
	if (bIsAutoPlaySound && CurrentAnimSound.ToSoftObjectPath().IsValid())
	{
		UAkComponent* AudioComp = GetAudioComponent();
		if (!AudioComp)
		{
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioWwise::Play. Playing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAkComponent. Actor: %s. Asset: %s")
				, *GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToSoftObjectPath().ToString());
			return false;
		}

		if (UAkAudioEvent* SoundEvent = GetAkEvent(GetOwner(), CurrentAnimSound))
		{
			//Clamp to audio duration if available
			//In order to have this data available within the sound events, you need to enable the metadata export within Wwise:
			// - In Wwise, go to Project -> Project Settings
			// - Select the SoundBanks tab
			// - Check these check boxes: "Generate Per Bank Metadata file", "Generate JSON Metadata", "Estimated duration"
			CurrentProgress = SoundEvent->MaximumDuration > 0.F ? FMath::Clamp(Position, 0.F, SoundEvent->MaximumDuration) : Position;

			if (!IsPlaying())
			{
				//start playback
				const int32 PlaybackId = AudioComp->PostAkEvent(SoundEvent, TEXT(""));
				if (PlaybackId == AK_INVALID_PLAYING_ID)
				{
					return false;
				}
			}

#ifdef WITH_WWISE_SEEK
#if WITH_EDITOR
			{
				static bool WarningPrinted = false;
				if (SoundEvent->MaximumDuration == 0.F && !WarningPrinted)
				{
					UE_LOG(LogFaceFX, Warning, TEXT("UAkAudioEvent is missing maximum duration. AkAudio may report exceeding seek range. Consider to enable soundbank metadata export for estimated duration within your Wwise project settings."));
					WarningPrinted = true;
				}
			}
#endif //WITH_EDITOR

			//Seek functionality within the current WWise UE4 plugin does not exist. Hence the playback from a specific location is not supported without changes to the Wwise plugin
			//See: https://www.audiokinetic.com/qa/3013/possible-to-seek-in-ue4-integration
			if (CurrentProgress > 0.F)
			{
				const bool SeekResult = AudioComp->Seek(CurrentProgress * 1000.F, SoundEvent, TEXT(""));
				if (!SeekResult)
				{
					return false;
				}
			}
#else //WITH_WWISE_SEEK
#pragma message ("warning : FaceFX Plugin: UAkComponent seek functionality missing. Audio playback may be wrong when using Wwise with FaceFX. Check FaceFX documentation for further details.")
			{
#if WITH_EDITOR
				static bool WarningPrinted = false;
				if (!WarningPrinted)
				{
					UE_LOG(LogFaceFX, Warning, TEXT("UAkComponent seek functionality missing. Audio playback may be wrong when using Wwise with FaceFX. Check FaceFX documentation for further details."));
					WarningPrinted = true;
				}
#endif //WITH_EDITOR
			}
#endif //WITH_WWISE_SEEK

			if (OutAudioComp)
			{
				*OutAudioComp = AudioComp;
			}

			PlaybackState = AudioComp->HasActiveEvents() ? EPlaybackState::Playing : EPlaybackState::Stopped;
			return IsPlaying();
		}
		else
		{
			UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioWwise::Play. Playing audio failed. Audio asset failed to load. Actor: %s. Asset: %s"),
				*GetNameSafe(GetOwningActor()), *CurrentAnimSound.ToSoftObjectPath().ToString());
		}
	}

	//no audio set or not to start
	return true;
}


bool FFaceFXAudioWwise::Pause(bool fadeOut)
{
	if (!IsPlaying())
	{
		//nothing playing right now
		return true;
	}

	if (UAkComponent* AudioComp = GetAudioComponent())
	{
		if (UAkAudioEvent* SoundEvent = GetAkEvent(GetOwner(), CurrentAnimSoundPause))
		{
			if (AudioComp->PostAkEvent(SoundEvent, TEXT("")) != AK_INVALID_PLAYING_ID)
			{
				PlaybackState = EPlaybackState::Paused;
			}
		}
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioWwise::Pause. Pausing audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAkComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool FFaceFXAudioWwise::Stop(bool enforceAudioCompStop)
{
	if (!IsPlayingOrPaused() && !enforceAudioCompStop)
	{
		//nothing playing right now
		return true;
	}

	if (UAkComponent* AudioComp = GetAudioComponent())
	{
		if (UAkAudioEvent* SoundEvent = GetAkEvent(GetOwner(), CurrentAnimSoundStop))
		{
			if (AudioComp->PostAkEvent(SoundEvent, TEXT("")) != AK_INVALID_PLAYING_ID)
			{
				CurrentProgress = 0.F;
				PlaybackState = EPlaybackState::Stopped;
			}
		}
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioWwise::Stop. Stopping audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAkComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}

bool FFaceFXAudioWwise::Resume()
{
	if (IsPlaying())
	{
		//already playing audio
		return true;
	}

	if (UAkComponent* AudioComp = GetAudioComponent())
	{
		if (UAkAudioEvent* SoundEvent = GetAkEvent(GetOwner(), CurrentAnimSoundResume))
		{
			if (AudioComp->PostAkEvent(SoundEvent, TEXT("")) != AK_INVALID_PLAYING_ID)
			{
				PlaybackState = EPlaybackState::Playing;
			}
		}
		return true;
	}

	UE_LOG(LogFaceFX, Error, TEXT("FFaceFXAudioWwise::Resume. Resuming audio failed. Owning UFaceFXComponent does not belong to an actor that owns an UAkComponent. Actor: %s."), *GetNameSafe(GetOwningActor()));
	return false;
}


bool FFaceFXAudioWwise::SetAudioComponent(UActorComponent* Component)
{
	if (UAkComponent* ComponentCast = Cast<UAkComponent>(Component))
	{
		AudioComponent = ComponentCast;
		return true;
	}
	return false;
}

UAkComponent* FFaceFXAudioWwise::GetAudioComponent() const
{
	if (UAkComponent* AudioComponentPtr = AudioComponent.Get())
	{
		return AudioComponentPtr;
	}

	AActor* OwningActor = GetOwningActor();
	return OwningActor ? OwningActor->FindComponentByClass<UAkComponent>() : nullptr;
}

#endif //WITH_WWISE
