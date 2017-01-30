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
	* @param fadeOut Indicator if the audio playback shall fade out quickly instead of stopping
	* @returns True if succeeded, else false
	*/
	bool Pause(bool fadeOut = false);

	/**
	* Stops the playback of this facial animation
	* @param enforceStop Indicator if the stop is enforced no matter of current state
	* @returns True if succeeded, else false
	*/
	bool Stop(bool enforceStop = false);

	/**
	* Restarts the current animation
	* @returns True if succeeded, else false
	*/
	inline bool Restart()
	{
		return JumpTo(0.F);
	}

	/**
	* Jumps to a given position within the facial animation playback
	* @param Position The target position to jump to (in seconds). Ranges from 0 to animation duration
	* @returns True if succeeded, else false
	*/
	bool JumpTo(float Position);

	/** Reset the whole character setup */
	void Reset();

	/**
	* Loads the character data from the given data set
	* @param Dataset The data set to load from
	* @param IsDisabledMorphTargets Indicator if the use of available morph targets shall be disabled
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFXActor* Dataset, bool IsDisabledMorphTargets);

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
		return AnimPlaybackState == EPlaybackState::Playing;
	}

	/**
	* Gets the indicator if the character is playing a facial animation right now or if one is paused
	* @returns True if playing, else false
	*/
	inline bool IsPlayingOrPaused() const
	{
		return AnimPlaybackState != EPlaybackState::Stopped;
	}

	/**
	* Gets the indicator if the character is currently playing an animation with the given animation id
	* @param AnimId The id to look for
	* @returns True if such an animation is currently playing, else false
	*/
	inline bool IsPlayingOrPaused(const FFaceFXAnimId& AnimId) const
	{
		//look for the animation by id. If no group is set for the given AnimId, ignore group during comparison
		const FFaceFXAnimId CurrentAnimId = GetCurrentAnimationId();
		return IsPlayingOrPaused() && ((!AnimId.Group.IsNone() && CurrentAnimId == AnimId) || AnimId.Name == CurrentAnimId.Name);
	}

	/**
	* Gets the indicator if the character is currently playing a given animation
	* @param Animation The animation to look for
	* @returns True if such an animation is currently playing, else false
	*/
	bool IsPlayingOrPaused(const class UFaceFXAnim* Animation) const;

	/**
	* Gets the indicator if this character is currently pausing a facial animation
	* @returns True If the character is having a paused facial animation, else false
	*/
	inline bool IsPaused() const
	{
		return AnimPlaybackState == EPlaybackState::Paused && CurrentAnimHandle;
	}

	/**
	* Gets the indicator if the character is playing a audio right now
	* @returns True if playing, else false
	*/
	inline bool IsPlayingAudio() const
	{
		return AudioPlaybackState == EPlaybackState::Playing;
	}

	/**
	* Gets the indicator if the audio is currently playing or paused
	* @returns True if paused or playing, else false
	*/
	inline bool IsPlayingOrPausedAudio() const
	{
		return AudioPlaybackState != EPlaybackState::Stopped;
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

	/**
	* Sets the audio component for this character
	* @param Component The new audio component
	*/
	inline void SetAudioComponent(class UAudioComponent* Component)
	{
		AudioComponent = Component;
	}

	//FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~FTickableGameObject

#if FACEFX_USEANIMATIONLINKAGE

	/**
	* Gets the start and end time of a given animation
	* @param Actor Contextual actor to fetch the FaceFX character from
	* @param AnimId The animation id to fetch the bounds for
	* @param OutStart The start time if call succeeded
	* @param OutEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationBoundsById(const AActor* Actor, const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd);

	/**
	* Gets the start and end time of a given animation
	* @param FaceFXActor The FaceFX Actor to retrieve the animation from
	* @param AnimId The animation id to fetch the bounds for
	* @param OutStart The start time if call succeeded
	* @param OutEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationBoundsById(const UFaceFXActor* FaceFXActor, const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd);

	/**
	* Gets the start and end time of a given animation
	* @param AnimId The animation id to fetch the bounds for
	* @param OutStart The start time if call succeeded
	* @param OutEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	inline bool GetAnimationBoundsById(const FFaceFXAnimId& AnimId, float& OutStart, float& OutEnd) const
	{
		return GetAnimationBoundsById(FaceFXActor, AnimId, OutStart, OutEnd);
	}

	/**
	* Gets the list of animation ids of all animations that are currently linked to this character
	* @param OutAnimIds The resulting list of animation ids
	* @returns True if call succeeded, else false
	*/
	bool GetAllLinkedAnimationIds(TArray<FFaceFXAnimId>& OutAnimIds) const;
#endif //FACEFX_USEANIMATIONLINKAGE

	/**
	* Gets the start and end time of a given animation
	* @param Animation The animation to fetch the bounds for
	* @param OutStart The start time if call succeeded
	* @param OutEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationBounds(const class UFaceFXAnim* Animation, float& OutStart, float& OutEnd);

	/** Event that triggers whenever an asset was tried to get played which is incompatible to the FaceFX actor handle */
	static FOnFaceFXCharacterPlayAssetIncompatibleSignature OnFaceFXCharacterPlayAssetIncompatible;

private:

	/** The different playback states */
	enum class EPlaybackState
	{
		Playing,
		Paused,
		Stopped
	};

	/**
	* Gets the currently playing animation
	* @returns The animation name
	*/
	FFaceFXAnimId GetCurrentAnimationId() const;

	/**
	* Gets the skel mesh component that owns this FaceFX character
	* @returns The owning skel mesh component or nullptr if not found
	*/
	class USkeletalMeshComponent* GetOwningSkelMeshComponent() const;

	/**
	* Gets the FaceFX component that owns this FaceFX character instance
	* @returns The FaceFX component or nullptr if there is no or another owner of this FaceFX character instance
	*/
	class UFaceFXComponent* GetOwningFaceFXComponent() const;

	/**
	* Gets the owning actor
	* @returns The actor or nullptr if not belonging to one
	*/
	class AActor* GetOwningActor() const;

	/**
	* Gets the audio component assigned to this character. If not set the audio component will be looked up from the owning actors component list
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
	* Gets the indicator if the audio start event was triggered within the current frame state
	* @returns True if audio was started, else false
	*/
	bool IsAudioStarted();

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
	inline bool PlayAudio(class UAudioComponent** OutAudioComp = nullptr)
	{
		return PlayAudio(0.F, OutAudioComp);
	}

	/**
	* Plays the audio if available
	* @param Position The position to start the audio at. Ranging from 0 to audio playback duration. Keep at 0 to start from the beginning. Will be clamped at 0
	* @param OutAudioComp The audio component on which audio was started to play. Unchanged if function returns false
	* @returns True if audio playback successfully started on the owning actors Audio component, else false
	*/
	bool PlayAudio(float Position = 0.F, class UAudioComponent** OutAudioComp = nullptr);

	/**
	* Pausing the playback of the currently playing audio
	* @param fadeOut Indicator if the playback shall fade out quickly instead of stopping
	* @returns True if succeeded, else false
	*/
	bool PauseAudio(bool fadeOut = false);

	/**
	* Stops the playback of the currently playing audio
	* @param enforceAudioCompStop Indicator if the stop on the audio component is enforced
	* @returns True if succeeded, else false
	*/
	bool StopAudio(bool enforceAudioCompStop = false);

	/**
	* Resumes the playback of the currently paused audio
	* @returns True if succeeded, else false
	*/
	bool ResumeAudio();

	/** Enforces a tick with a zero delta */
	inline void EnforceZeroTick()
	{
#if WITH_EDITOR
		LastFrameNumber = GFrameNumber+1;
#endif //WITH_EDITOR
		Tick(0.F);
	}

	/**
	* Performs ticks from 0 to Duration in small enough timesteps to find out the location where the audio was triggered
	* @param Duration The duration until to tick to
	* @param OutAudioStarted True if the audio was started until the duration was reached, else false
	* @returns True if succeeded with ticking until the duration, else false
	*/
	bool TickUntil(float Duration, bool& OutAudioStarted);

	/**
	* Retrieves the morph targets for a skel mesh and creates FaceFX indices for the names
	* @returns True if setup succeeded, else false
	*/
	bool SetupMorphTargets();

	/** Processes the morph targets for the current frame state */
	void ProcessMorphTargets();

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
	UPROPERTY(Transient)
	const UFaceFXActor* FaceFXActor;

	/** The audio component assigned to this character */
	UPROPERTY(Transient)
	class UAudioComponent* AudioComponent;

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

	/** The list of morph target names retrieved from the skel mesh during asset loading. The indices match the morph target track values: MorphTargetTrackValues */
	TArray<FName> MorphTargetNames;

	/** The track values buffer for the morph target processing */
	TArray<ffx_track_value_t> MorphTargetTrackValues;

	/** The overall time progression */
	float CurrentTime;

	/** The progression of the currently playing animation */
	float CurrentAnimProgress;

	/** The total duration of the currently playing animation */
	float CurrentAnimDuration;

	/** The starting location of the currently playing animation */
	float CurrentAnimStart;

	/** The location at which we are right now on the audio playback (in seconds) */
	float CurrentAudioProgress;

	/** The animation asset of the currently playing animation */
	UPROPERTY(Transient)
	const class UFaceFXAnim* CurrentAnim;

	/** The current audio asset that was assigned to the current animation*/
	TAssetPtr<class USoundWave> CurrentAnimSound;

	/** The animation playback state */
	EPlaybackState AnimPlaybackState;

	/** The audio playback state */
	EPlaybackState AudioPlaybackState;

	/** Dirty indicator */
	uint8 bIsDirty : 1;

	/** Looping indicator for the currently playing animation */
	uint8 bIsLooping : 1;

	/** Indicator if this character is allowed to play */
	uint8 bCanPlay : 1;

	/** Indicator that defines if the FaceFX character shall play the sound wave assigned to the FaceFX Animation asset automatically when this animation is getting played */
	uint8 bIsAutoPlaySound : 1;

	/** Indicator if the use of available morph targets shall be disabled */
	uint8 bDisabledMorphTargets : 1;

#if WITH_EDITOR
	uint32 LastFrameNumber;

	/** The event callback handle for OnFaceFXAnimChanged */
	FDelegateHandle OnFaceFXAnimChangedHandle;

	/**
	* Callback for when an asset changed
	* @param Asset The asset which changed
	*/
	void OnFaceFXAssetChanged(class UFaceFXAsset* Asset);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetChangedSignature, class UFaceFXAsset* /*Asset*/);

public:
	/** Event that gets triggered when an animation asset gets loaded */
	static FOnAssetChangedSignature OnAssetChanged;
#endif //WITH_EDITOR
};
