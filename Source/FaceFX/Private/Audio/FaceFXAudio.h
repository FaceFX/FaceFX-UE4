/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2023 OC3 Entertainment, Inc. All rights reserved.
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

#pragma once

#include "FaceFXData.h"

class UActorComponent;
class UFaceFXCharacter;
class UFaceFXAnim;

/** Interface for an Audio system that implements the audio features used by the FaceFX plugin */
struct IFaceFXAudio
{
	virtual ~IFaceFXAudio(){}

	/** 
	* Ticks the player
	* @param DeltaTime The time passed since the last tick
	*/
	inline void Tick(float DeltaTime)
	{
		if (IsPlaying())
		{
			CurrentProgress += DeltaTime;
		}
	}

	/**
	* Prepares the audio data if needed for the current animation
	* @param Animation The animation to prepare the audio for
	*/
	virtual void Prepare(const UFaceFXAnim* Animation) {}

	/**
	* Plays the audio if available
	* @param OutAudioComp The audio component on which audio was started to play. Unchanged if function returns false
	* @returns True if audio playback successfully started on the owning actors Audio component, else false
	*/
	inline bool Play(UActorComponent** OutAudioComp = nullptr)
	{
		return Play(0.F, OutAudioComp);
	}

	/**
	* Plays the audio if available
	* @param Position The position to start the audio at. Ranging from 0 to audio playback duration. Keep at 0 to start from the beginning. Will be clamped at 0
	* @param OutAudioComp The audio component on which audio was started to play. Unchanged if function returns false
	* @returns True if audio playback successfully started on the owning actors Audio component, else false
	*/
	virtual bool Play(float Position = 0.F, UActorComponent** OutAudioComp = nullptr) = 0;

	/**
	* Pausing the playback of the currently playing audio
	* @param fadeOut Indicator if the playback shall fade out quickly instead of stopping
	* @returns True if succeeded, else false
	*/
	virtual bool Pause(bool fadeOut = false) = 0;

	/**
	* Stops the playback of the currently playing audio
	* @param enforceAudioCompStop Indicator if the stop on the audio component is enforced
	* @returns True if succeeded, else false
	*/
	virtual bool Stop(bool enforceAudioCompStop = false) = 0;

	/**
	* Resumes the playback of the currently paused audio
	* @returns True if succeeded, else false
	*/
	virtual bool Resume() = 0;

	/**
	* Sets the audio component for the player
	* @param Component The new audio component
	*/
	virtual bool SetAudioComponent(UActorComponent* Component) 
	{ 
		return true; 
	}

	/** 
	* Gets the location at which we are right now on the audio playback (in seconds)
	* @returns The location in seconds
	*/
	inline float GetCurrentProgress() const
	{
		return CurrentProgress;
	}

	/**
	* Gets the indicator if the audio shall be played automatically if available
	* @returns True if auto play is enabled, else false
	*/
	inline bool IsAutoPlaySound() const
	{
		return bIsAutoPlaySound;
	}

	/**
	* Sets the indicator if the audio shall be played automatically if available
	* @param isAutoPlaySound The new indicator value
	*/
	inline void SetAutoPlaySound(bool isAutoPlaySound)
	{
		bIsAutoPlaySound = isAutoPlaySound;
	}

	/**
	* Gets the indicator if the character is playing a audio right now
	* @returns True if playing, else false
	*/
	inline bool IsPlaying() const
	{
		return PlaybackState == EPlaybackState::Playing;
	}

	/**
	* Gets the indicator if the audio is currently playing or paused
	* @returns True if paused or playing, else false
	*/
	inline bool IsPlayingOrPaused() const
	{
		return PlaybackState != EPlaybackState::Stopped;
	}

	/**
	* Gets the indicator if the audio is currently paused
	* @returns True if paused, else false
	*/
	inline bool IsPaused() const
	{
		return PlaybackState != EPlaybackState::Paused;
	}

	/**
	* Gets the indicator if the audio is currently stopped
	* @returns True if stopped, else false
	*/
	inline bool IsStopped() const
	{
		return PlaybackState == EPlaybackState::Stopped;
	}

	/** 
	* Gets the owning component of this audio player 
	* @returns The owning component or nullptr
	*/
	UFaceFXCharacter* GetOwner() const
	{
		return Owner.Get();
	}

	/**
	* Gets the owning actor
	* @returns The actor or nullptr if not belonging to one
	*/
	AActor* GetOwningActor() const;

protected:

	IFaceFXAudio(UFaceFXCharacter* InOwner) : Owner(InOwner), CurrentProgress(0.F), PlaybackState(EPlaybackState::Stopped), bIsAutoPlaySound(false) {}

	/** The owning component */
	TWeakObjectPtr<UFaceFXCharacter> Owner;

	/** The location at which we are right now on the audio playback (in seconds) */
	float CurrentProgress;

	/** The audio playback state */
	EPlaybackState PlaybackState;

	/** Indicator that defines if the FaceFX character shall play the sound wave assigned to the FaceFX Animation asset automatically when an animation is getting played */
	uint8 bIsAutoPlaySound : 1;
};

/** Main audio layer */
struct FFaceFXAudio
{	
	/** 
	* Creates the audio system to be used 
	* @param Owner The owning character component to create the player for
	*/
	static TSharedPtr<IFaceFXAudio> Create(UFaceFXCharacter* Owner);

	/** 
	* Gets the indicator if the active audio system is using sound wave assets
	* @returns True if sound wave assets are used, else false
	*/
	static FACEFX_API bool IsUsingSoundWaveAssets();

private:

	FFaceFXAudio(){}
};