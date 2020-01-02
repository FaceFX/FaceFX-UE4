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

#pragma once

#include "FaceFXConfig.h"
#include "FaceFXData.h"
#include "FaceFXAnim.h"

#include "Tickable.h"
#include "FaceFXCharacter.generated.h"

struct IFaceFXAudio;
class UFaceFXActor;
class UFaceFXComponent;
class UFaceFXAsset;
class AActor;

/** Class that represents a FaceFX character instance */
UCLASS()
class FACEFX_API UFaceFXCharacter : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

	/** The delegate used for various FaceFX events */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFaceFXCharacterEventSignature, UFaceFXCharacter* /*Character*/, const FFaceFXAnimId& /*AnimId*/);
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnFaceFXCharacterAudioStartEventSignature, UFaceFXCharacter* /*Character*/, const FFaceFXAnimId& /*AnimId*/, bool /*IsAudioStarted*/, UActorComponent* /*AudioComponentStartedOn*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFaceFXCharacterPlayAssetIncompatibleSignature, UFaceFXCharacter* /*Character*/, const UFaceFXAnim* /*Asset*/);

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
	bool Play(const UFaceFXAnim* Animation, bool Loop = false);

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
	* @param IsCompensateForForceFrontXAxis Indicator that compensates for the Force Front XAxis setting when importing FBX files
	* @param IsDisabledMorphTargets Indicator if the use of available morph targets shall be disabled
	* @param IsDisableMaterialParameters Indicator if the use of material parameters shall be disabled
	* @returns True if succeeded, else false
	*/
	bool Load(const UFaceFXActor* Dataset, bool IsCompensateForForceFrontXAxis, bool IsDisabledMorphTargets, bool IsDisableMaterialParameters);

	/**
	* Gets the indicator if this character have been loaded
	* @returns True if loaded else false
	*/
	inline bool IsLoaded() const
	{
		return ActorHandle != nullptr && FaceFXActor;
	}

	/**
	* Gets the indicator if the character has the given facial animation active right now (playing or not)
	* @param AnimId The animation ID we check for
	* @returns True if the given animation is active, else false
	*/
	inline bool IsAnimationActive(const FFaceFXAnimId& AnimId) const
	{
		//look for the animation by id. If no group is set for the given AnimId, ignore group during comparison
		const FFaceFXAnimId CurrentAnimId = GetCurrentAnimationId();
		return ((!AnimId.Group.IsNone() && CurrentAnimId == AnimId) || AnimId.Name == CurrentAnimId.Name);
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
	* Gets the indicator if the character is playing the given facial animation right now
	* @param AnimId The animation ID we check for playback
	* @returns True if playing the given animation, else false
	*/
	inline bool IsPlaying(const FFaceFXAnimId& AnimId) const
	{
		return IsPlaying() && IsAnimationActive(AnimId);
	}

	/**
	* Gets the indicator if the character is playing the given facial animation right now
	* @param Animation The animation we check for playback
	* @returns True if playing the given animation, else false
	*/
	bool IsPlaying(const UFaceFXAnim* Animation) const;

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
		return IsPlayingOrPaused() && IsAnimationActive(AnimId);
	}

	/**
	* Gets the indicator if the character is currently playing a given animation
	* @param Animation The animation to look for
	* @returns True if such an animation is currently playing, else false
	*/
	bool IsPlayingOrPaused(const UFaceFXAnim* Animation) const;

	/**
	* Gets the indicator if this character is currently pausing a facial animation
	* @returns True If the character is having a paused facial animation, else false
	*/
	inline bool IsPaused() const
	{
		return AnimPlaybackState == EPlaybackState::Paused && CurrentAnimHandle;
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
	* Gets the indicator if the character is playing a audio right now
	* @returns True if playing, else false
	*/
	bool IsPlayingAudio() const;

	/**
	* Gets the indicator if the audio is currently playing or paused
	* @returns True if paused or playing, else false
	*/
	bool IsPlayingOrPausedAudio() const;

	/**
	* Gets the indicator if the audio shall be played automatically if available
	* @returns True if auto play is enabled, else false
	*/
	bool IsAutoPlaySound() const;

	/**
	* Sets the indicator if the audio shall be played automatically if available
	* @param isAutoPlaySound The new indicator value
	*/
	void SetAutoPlaySound(bool isAutoPlaySound);

	/**
	* Sets the audio component for this character
	* @param Component The new audio component
	*/
	void SetAudioComponent(UActorComponent* Component);

	/** 
	* Gets the audio player associated with this character 
	* @returns The audio player
	*/
	inline IFaceFXAudio* GetAudioPlayer() const
	{
		return AudioPlayer.Get();
	}

	inline EFaceFXBlendMode GetBlendMode() const
	{
		return BlendMode;
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
	* Gets the owning actor
	* @returns The actor or nullptr if not belonging to one
	*/
	AActor* GetOwningActor() const;

	//FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override
	{
		return true;
	}
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

	/** Event that triggers whenever an asset was tried to get played which is incompatible to the FaceFX actor handle */
	static FOnFaceFXCharacterPlayAssetIncompatibleSignature OnFaceFXCharacterPlayAssetIncompatible;

private:

	/**
	* Gets the currently playing animation
	* @returns The animation name
	*/
	FFaceFXAnimId GetCurrentAnimationId() const;

	/**
	* Gets the skel mesh component that owns this FaceFX character
	* @returns The owning skel mesh component or nullptr if not found
	*/
	USkeletalMeshComponent* GetOwningSkelMeshComponent() const;

	/**
	* Gets the FaceFX component that owns this FaceFX character instance
	* @returns The FaceFX component or nullptr if there is no or another owner of this FaceFX character instance
	*/
	UFaceFXComponent* GetOwningFaceFXComponent() const;

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
	* @param Dataset The asset to fetch the ids from
	* @returns True if setup succeeded, else false
	*/
	bool SetupMorphTargets(const UFaceFXActor* Dataset);
	
	/** Processes the morph targets for the current frame state */
	void ProcessMorphTargets();

	/** Resets the current morph target data */
	inline void ResetMorphTargets()
	{
		MorphTargetNames.Empty();
		MorphTargetTrackValues.Empty();
	}

	/**
	* Retrieves the material parameters for skel mesh materials and creates FaceFX indices for the names
	* @param Dataset The asset to fetch the ids from
	* @param IgnoredTracks The list of tracks to ignore
	* @returns True if setup succeeded, else false
	*/
	bool SetupMaterialParameters(const UFaceFXActor* Dataset, const TArray<FName>& IgnoredTracks);

	/** Processes the material parameters for the current frame state */
	void ProcessMaterialParameters();

	/** Resets the current material parameter data */
	inline void ResetMaterialParameters()
	{
		MaterialParameterNames.Empty();
		MaterialParameterTrackValues.Empty();
	}

	/** Sets the material parameters of the owners skel mesh to their defaults */
	void ResetMaterialParametersToDefaults();

	/** The data set from where this character was loaded from */
	UPROPERTY(Transient)
	const UFaceFXActor* FaceFXActor;

	/** The audio player for this character */
	TSharedPtr<IFaceFXAudio> AudioPlayer;

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

	/** The list of material parameter names retrieved from the skel mesh during asset loading. The indices match the material parameter track values: MaterialParameterTrackValues */
	TArray<FName> MaterialParameterNames;

	/** The track values buffer for the material parameter processing */
	TArray<ffx_track_value_t> MaterialParameterTrackValues;

	/** The overall time progression */
	float CurrentTime;

	/** The progression of the currently playing animation */
	float CurrentAnimProgress;

	/** The total duration of the currently playing animation */
	float CurrentAnimDuration;

	/** The starting location of the currently playing animation */
	float CurrentAnimStart;

	/** The animation asset of the currently playing animation */
	UPROPERTY(Transient)
	const UFaceFXAnim* CurrentAnim;

	/** The animation playback state */
	EPlaybackState AnimPlaybackState;

	/** Used blend mode. Either defined by global config or overriden via FaceFXActor */
	EFaceFXBlendMode BlendMode;

	/** Dirty indicator */
	uint8 bIsDirty : 1;

	/** Looping indicator for the currently playing animation */
	uint8 bIsLooping : 1;

	/** Indicator if this character is allowed to play */
	uint8 bCanPlay : 1;

	/** Indicator if compensate for force front x axis was used. */
	uint8 bCompensatedForForceFrontXAxis : 1;

	/** Indicator if the use of available morph targets shall be disabled */
	uint8 bDisabledMorphTargets : 1;

	/** Indicator if the use of available material parameters shall be disabled */
	uint8 bDisabledMaterialParameters : 1;

#if WITH_EDITOR
	uint32 LastFrameNumber;

	/** The event callback handle for OnFaceFXAnimChanged */
	FDelegateHandle OnFaceFXAnimChangedHandle;

	/**
	* Callback for when an asset changed
	* @param Asset The asset which changed
	*/
	void OnFaceFXAssetChanged(UFaceFXAsset* Asset);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetChangedSignature, UFaceFXAsset* /*Asset*/);

public:
	/** Event that gets triggered when an animation asset gets loaded */
	static FOnAssetChangedSignature OnAssetChanged;
#endif //WITH_EDITOR
};
