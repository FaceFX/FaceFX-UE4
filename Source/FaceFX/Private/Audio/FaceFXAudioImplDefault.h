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

#pragma once

#include "FaceFXAudio.h"
#include "Components/AudioComponent.h"

class UActorComponent;

/** Audio layer that uses the Unreal Audio System */
struct FFaceFXAudioDefault : public IFaceFXAudio
{
	FFaceFXAudioDefault(UFaceFXCharacter* InOwner) : IFaceFXAudio(InOwner), AudioComponent(nullptr) {}

	/**
	* Prepares the audio data if needed for the current animation
	* @param Animation The animation to prepare the audio for
	*/
	virtual void Prepare(const UFaceFXAnim* Animation) override;

	/**
	* Plays the audio if available
	* @param Position The position to start the audio at. Ranging from 0 to audio playback duration. Keep at 0 to start from the beginning. Will be clamped at 0
	* @param OutAudioComp The audio component on which audio was started to play. Unchanged if function returns false
	* @returns True if audio playback successfully started on the owning actors Audio component, else false
	*/
	virtual bool Play(float Position = 0.F, UActorComponent** OutAudioComp = nullptr) override;
	
	/**
	* Pausing the playback of the currently playing audio
	* @param fadeOut Indicator if the playback shall fade out quickly instead of stopping
	* @returns True if succeeded, else false
	*/
	virtual bool Pause(bool fadeOut = false) override;

	/**
	* Stops the playback of the currently playing audio
	* @param enforceAudioCompStop Indicator if the stop on the audio component is enforced
	* @returns True if succeeded, else false
	*/
	virtual bool Stop(bool enforceAudioCompStop = false) override;

	/**
	* Resumes the playback of the currently paused audio
	* @returns True if succeeded, else false
	*/
	virtual bool Resume() override;

	/**
	* Sets the audio component for the player
	* @param Component The new audio component
	*/
	virtual bool SetAudioComponent(UActorComponent* Component) override;

private:

	/**
	* Gets the audio component assigned to the owning character. If not set the audio component will be looked up from the owning actors component list
	* @returns The audio component or nullptr if not found
	*/
	UAudioComponent* GetAudioComponent() const;

	/** The audio component assigned to the character */
	TWeakObjectPtr<UAudioComponent> AudioComponent;

	/** The current audio asset that was assigned to the current animation*/
	TSoftObjectPtr<USoundWave> CurrentAnimSound;
};
