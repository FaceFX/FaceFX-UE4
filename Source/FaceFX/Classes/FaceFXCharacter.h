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

#pragma once

#include "FaceFXConfig.h"
#include "FaceFXData.h"
#include "Tickable.h"
#include "FaceFXCharacter.generated.h"

/** The delegate used for various FaceFX events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFaceFXCharacterEventSignature, class UFaceFXCharacter*, Character, const struct FFaceFXAnimId&, AnimId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnFaceFXCharacterAudioStartEventSignature, class UFaceFXCharacter*, Character, const struct FFaceFXAnimId&, AnimId, bool, IsAudioStarted, class UAudioComponent*, AudioComponentStartedOn);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFaceFXCharacterPlayAssetIncompatibleSignature, class UFaceFXCharacter* /*Character*/, const class UFaceFXAnim* /*Asset*/);

/** Class that represents a FaceFX character instance */
UCLASS()
class FACEFX_API UFaceFXCharacter : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

public:

	//UObject
	virtual void BeginDestroy() override;
	//~UObject

	/** Event that triggers whenever this characters currently playing animation request audio playback */
	FOnFaceFXCharacterAudioStartEventSignature OnPlaybackStartAudio;

	/** Event that triggers whenever this character stops playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackStopped;

	/** Event that triggers whenever this character started playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackStarted;

	/** Event that triggers whenever this character paused playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackPaused;

#if FACEFX_USEANIMATIONLINKAGE
	/**
	* Starts the playback of the given facial animation
	* @param AnimName The animation to play
	* @param AnimGroup The animation group to find the animation in
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	inline bool Play(const FName& AnimName, const FName& AnimGroup = NAME_None, bool Loop = false)
	{
		return Play(FFaceFXAnimId(AnimGroup, AnimName), Loop);
	}

	/**
	* Starts the playback of the given facial animation
	* @param AnimId The animation to play
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	bool Play(const FFaceFXAnimId& AnimId, bool Loop = false);

#endif //FACEFX_USEANIMATIONLINKAGE

	/**
	* Starts the playback of the given facial animation asset
	* @param Animation The animation to play
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	bool Play(const class UFaceFXAnim* Animation, bool Loop = false);

	/**
	* Resumes the playback of the facial animation
	* @returns True if succeeded, else false
	*/
	bool Resume();

	/**
	* Pauses the playback of the facial animation
	* @returns True if succeeded, else false
	*/
	bool Pause();

	/**
	* Stops the playback of this facial animation
	* @returns True if succeeded, else false
	*/
	bool Stop();

	/**
	* Restarts the current animation
	* @returns True if succeeded, else false
	*/
	bool Restart();

	/** Reset the whole character setup */
	void Reset();

	/**
	* Loads the character data from the given data set
	* @param Dataset The data set to load from
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFXActor* Dataset);

	/**
	* Gets the indicator if this character have been loaded
	* @returns True if loaded else false
	*/
	inline bool IsLoaded() const
	{
		return ActorHandle != nullptr && FaceFXActor;
	}

	/**
	* Gets the indicator if the character is playing a facial animation right now
	* @returns True if playing, else false
	*/
	inline bool IsPlaying() const
	{
		return bIsPlaying;
	}

	/** 
	* Gets the indicator if this character is currently pausing a facial animation
	* @returns True If the character is having a paused facial animation, else false
	*/
	inline bool IsPaused() const
	{
		return !bIsPlaying && CurrentAnimHandle;
	}

	/**
	* Gets the indicator if the character is playing a audio right now
	* @returns True if playing, else false
	*/
	inline bool IsPlayingAudio() const
	{
		return bIsPlayingAudio;
	}

	/**
	* Gets the indicator if the current animation is looping
	* @returns True if looping else false
	*/
	inline bool IsLooping() const
	{
		return bIsLooping;
	}

	/** 
	* Checks if the character FaceFX actor handle can play the given animation
	* @param Animation The animation to check
	* @returns True if it can play the animation, else false
	*/
	bool IsCanPlay(const UFaceFXAnim* Animation) const;

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
	* Gets the list of bone names
	* @returns The list of bone names
	*/
	inline const TArray<FName>& GetBoneNames() const
	{
		return BoneNames;
	}

	/**
	* Gets the index within the transforms for a given bone name
	* @param Name The bone name to look for
	* @returns The transforms index, INDEX_NONE if not found
	*/
	int32 GetBoneNameTransformIndex(const FName& Name) const;

	/**
	* Receives the current bone transforms
	* @param UpdateIfDirty Indicator if the transforms shall be updated if the character is dirty
	* @return The current bone transforms
	*/
	inline const TArray<FTransform>& GetBoneTransforms(bool UpdateIfDirty = true)
	{
		if(bIsDirty && UpdateIfDirty)
		{
			UpdateTransforms();
		}

		return BoneTransforms;
	}

	/**
	* Gets the assigned FaceFX actor asset
	* @returns The assigned FaceFX actor asset
	*/
	inline const UFaceFXActor* GetFaceFXActor() const
	{
		return FaceFXActor;
	}

	//FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~FTickableGameObject

	/** Event that triggers whenever an asset was tried to get played which is incompatible to the FaceFX actor handle */
	static FOnFaceFXCharacterPlayAssetIncompatibleSignature OnFaceFXCharacterPlayAssetIncompatible;

private:

	/**
	* Gets the currently playing animation
	* @returns The animation name
	*/
	inline const FFaceFXAnimId& GetCurrentAnimation() const
	{
		return CurrentAnim;
	}

	/** 
	* Gets the owning actor
	* @returns The actor or nullptr if not belonging to one
	*/
	class AActor* GetOwnningActor() const;

	/** 
	* Gets the audio component from the owning actor
	* @returns The audio component or nullptr if not found
	*/
	class UAudioComponent* GetAudioComponent() const;

	/** 
	* Checks if the character FaceFX actor handle can play the given animation handle
	* @param AnimationHandle The animation handle to check
	* @returns True if it can play the animation, else false
	*/
	bool IsCanPlay(struct ffx_anim_handle_t* AnimationHandle) const;

	/** 
	* Gets the start and end time of the current animation
	* @param OutStart The start time if call succeeded
	* @param OutEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	bool GetAnimationBounds(float& OutStart, float& OutEnd) const;

	/**
	* Updates the facial animation
	* @param DeltaTime The time passed since the last update
	* @returns True if succeeded, else false
	*/
	bool Update(float DeltaTime);

	/** Updates the local bone transform table */
	void UpdateTransforms();

	/**
	* Unload the current animation
	*/
	void UnloadCurrentAnim();

	/** 
	* Prepares the audio data if needed for the current animation 
	* @param Animation The animation to prepare the audio for
	*/
	void PrepareAudio(const UFaceFXAnim* Animation);

	/** 
	* Plays the audio if available 
	* @param OutAudioComp The audio component on which audio was started to play. Unchanged if function returns false
	* @returns True if audio playback successfully started on the owning actors Audio component, else false
	*/
	bool PlayAudio(class UAudioComponent** OutAudioComp = nullptr);

	/** 
	* Pausing the playback of the currently playing audio
	* @returns True if succeeded, else false
	*/
	bool PauseAudio();

	/** 
	* Stops the playback of the currently playing audio
	* @returns True if succeeded, else false
	*/
	bool StopAudio();

	/** 
	* Resumes the playback of the currently paused audio
	* @returns True if succeeded, else false
	*/
	bool ResumeAudio();

	/**
	* Gets the latest internal facefx error message
	* @returns The last error message
	*/
	static FString GetFaceFXError();

	/**
	* Loads a set of animation data
	* @param AnimData The data to load the animation with
	* @returns The FaceFX handle if succeeded, else nullptr
	*/
	static struct ffx_anim_handle_t* LoadAnimation(const struct FFaceFXAnimData& AnimData);

	/** The data set from where this character was loaded from */
	UPROPERTY()
	const UFaceFXActor* FaceFXActor;

	/** The associated actor handle */
	struct ffx_actor_handle_t* ActorHandle;

	/** The current frame state */
	struct ffx_frame_state_t* FrameState;

	/** The bone set handle */
	struct ffx_bone_set_handle_t* BoneSetHandle;

	/** The handle of the currently playing animation */
	struct ffx_anim_handle_t* CurrentAnimHandle;

	/** The currently loaded xforms */
	TArray<ffx_bone_xform_t> XForms;

	/** The current bone transforms */
	TArray<FTransform> BoneTransforms;

	/** The list of bone names defined within the FaceFX data used by this character */
	TArray<FName> BoneNames;

	/** The bone ids coming from the facefx asset */
	TArray<uint64> BoneIds;

	/** The overall time progression */
	float CurrentTime;

	/** The progression of the currently playing animation */
	float CurrentAnimProgress;

	/** The total duration of the currently playing animation */
	float CurrentAnimDuration;

	/** The location at which we are right now on the audio playback (in seconds) */
	float CurrentAudioProgress;

	/** The id of the currently played animation */
	FFaceFXAnimId CurrentAnim;

	/** The current audio asset that was assigned to the current animation*/
	TAssetPtr<class USoundWave> CurrentAnimSound;

	/** Dirty indicator */
	uint8 bIsDirty : 1;

	/** Playing indicator */
	uint8 bIsPlaying : 1;

	/** Audio Playing indicator */
	uint8 bIsPlayingAudio : 1;

	/** Looping indicator for the currently playing animation */
	uint8 bIsLooping : 1;

	/** Indicator if this character is allowed to play */
	uint8 bCanPlay : 1;

	/** Indicator that defines if the FaceFX character shall play the sound wave assigned to the FaceFX Animation asset automatically when this animation is getting played */
	uint8 bIsAutoPlaySound : 1;

#if WITH_EDITOR
	uint32 LastFrameNumber;
#endif //WITH_EDITOR
};
