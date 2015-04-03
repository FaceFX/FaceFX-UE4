#pragma once

#include FACEFX_RUNTIMEHEADER
#include "FaceFXConfig.h"
#include "FaceFXData.h"
#include "Tickable.h"
#include "FaceFXCharacter.generated.h"

/** The delegate used for various FaceFX events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFaceFXCharacterEventSignature, class UFaceFXCharacter*, Character, const struct FFaceFXAnimId&, AnimId);

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
	FOnFaceFXCharacterEventSignature OnPlaybackStartAudio;

	/** Event that triggers whenever this character stops playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackStopped;

	/** Event that triggers whenever this character started playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackStarted;

	/** Event that triggers whenever this character paused playing an animation */
	FOnFaceFXCharacterEventSignature OnPlaybackPaused;

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

#if FACEFX_WITHBONEFILTER
	/**
	* Loads the character data from the given data set
	* @param Dataset The data set to load from
	* @param BoneFilter The bone filter to apply. Keep nullptr to apply the whole bone set as a filter
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFXActor* Dataset, const TArray<FName>* BoneFilter = nullptr);
#else
	/**
	* Loads the character data from the given data set
	* @param Dataset The data set to load from
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFXActor* Dataset);
#endif //FACEFX_WITHBONEFILTER

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
	* Gets the indicator if the current animation is looping
	* @returns True if looping else false
	*/
	inline bool IsLooping() const
	{
		return bIsLooping;
	}

#if FACEFX_WITHBONEFILTER

	/**
	* Gets the names of the bones that are currently filtered
	* @param OutResult The resulting bone names
	*/
	void GetFilteredBoneNames(TArray<FName>& OutResult) const;

	/**
	* Sets the bones we want to get the transforms for within GetBoneTransforms. The order will be defined by this
	* @param InBonesNames The names of the bones to load
	* @returns True if succeeded, else false (i.e. when the character have not been loaded yet)
	*/
	bool SetBoneFilter(const TArray<FName>& InBonesNames);

#else

	/**
	* Gets the list of bone names
	* @returns The list of bone names
	*/
	inline const TArray<FName>& GetBoneNames() const
	{
		return BoneNames;
	}
#endif //FACEFX_WITHBONEFILTER

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

	//FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~FTickableGameObject

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

#if FACEFX_WITHBONEFILTER
	/** The bone transform filter indices */
	TArray<int32> BoneTransformsFilterIndices;
#else
	/** The list of bone names defined within the FaceFX data used by this character */
	TArray<FName> BoneNames;
#endif //FACEFX_WITHBONEFILTER

	/** The bone ids coming from the facefx asset */
	TArray<uint64> BoneIds;

	/** The overall time progression */
	float CurrentTime;

	/** The progression of the currently playing animation */
	float CurrentAnimProgress;

	/** The total duration of the currently playing animation */
	float CurrentAnimDuration;

	/** The id of the currently played animation */
	FFaceFXAnimId CurrentAnim;

	/** Dirty indicator */
	uint8 bIsDirty : 1;

	/** Playing indicator */
	uint8 bIsPlaying : 1;

	/** Looping indicator for the currently playing animation */
	uint8 bIsLooping : 1;

	/** Indicator if this character is allowed to play */
	uint8 bCanPlay : 1;

#if WITH_EDITOR
	uint32 LastFrameNumber;
#endif //WITH_EDITOR
};