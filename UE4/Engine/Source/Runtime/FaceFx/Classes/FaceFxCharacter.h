#pragma once

#include "FaceFxConfig.h"
#include "FaceFxData.h"
#include "Tickable.h"
#include "facefx.h"
#include "FaceFxCharacter.generated.h"

/** Class that represents a FaceFX character instance */
UCLASS()
class FACEFX_API UFaceFxCharacter : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

public:

	virtual ~UFaceFxCharacter();

	/**
	* Starts the playback of the given facial animation
	* @param animName The animation to play
	* @param animGroup The animation group to find the animation in
	* @param loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	inline bool Play(const FName& animName, const FName& animGroup = NAME_None, bool loop = false)
	{
		return Play(FFaceFxAnimId(animGroup, animName), loop);
	}

	/**
	* Starts the playback of the given facial animation
	* @param animId The animation to play
	* @param loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	bool Play(const FFaceFxAnimId& animId, bool loop = false);

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
	* @param dataset The data set to load from
	* @param boneFilter The bone filter to apply. Keep nullptr to apply the whole bone set as a filter
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFxActor* dataset, const TArray<FName>* boneFilter = nullptr);
#else
	/**
	* Loads the character data from the given data set
	* @param dataset The data set to load from
	* @returns True if succeeded, else false
	*/
	bool Load(const class UFaceFxActor* dataset);
#endif //FACEFX_WITHBONEFILTER

	/**
	* Gets the indicator if this character have been loaded
	* @returns True if loaded else false
	*/
	inline bool IsLoaded() const
	{
		return m_actor_handle != nullptr && m_faceFxSet;
	}

	/**
	* Gets the indicator if the character is playing a facial animation right now
	* @returns True if playing, else false
	*/
	inline bool IsPlaying() const
	{
		return m_isPlaying;
	}

	/**
	* Gets the indicator if the current animation is looping
	* @returns True if looping else false
	*/
	inline bool IsLooping() const
	{
		return m_isLooping;
	}

#if FACEFX_WITHBONEFILTER

	/**
	* Gets the names of the bones that are currently filtered
	* @param outResult The resulting bone names
	*/
	void GetFilteredBoneNames(TArray<FName>& outResult) const;

	/**
	* Sets the bones we want to get the transforms for within GetBoneTransforms. The order will be defined by this
	* @param bonesNames The names of the bones to load
	* @returns True if succeeded, else false (i.e. when the character have not been loaded yet)
	*/
	bool SetBoneFilter(const TArray<FName>& bonesNames);

#else

	/**
	* Gets the list of bone names
	* @returns The list of bone names
	*/
	inline const TArray<FName>& GetBoneNames() const
	{
		return m_boneNames;
	}
#endif //FACEFX_WITHBONEFILTER

	/**
	* Gets the index within the transforms for a given bone name
	* @param name The bone name to look for
	* @returns The transforms index, INDEX_NONE if not found
	*/
	int32 GetBoneNameTransformIndex(const FName& name) const;

	/**
	* Receives the current bone transforms
	* @return The current bone transforms
	*/
	inline const TArray<FTransform>& GetBoneTransforms()
	{
		if(m_isDirty)
		{
			UpdateTransforms();
		}

		return m_boneTransforms;
	}

	//FTickableGameObject
	virtual void Tick(float deltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~FTickableGameObject

private:

	/**
	* Gets the currently playing animation
	* @returns The animation name
	*/
	inline const FFaceFxAnimId& GetCurrentAnimation() const
	{
		return m_currentAnim;
	}

	/** 
	* Gets the start and end time of the current animation
	* @param outStart The start time if call succeeded
	* @param outEnd The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	bool GetAnimationBounds(float& outStart, float& outEnd) const;

	/**
	* Updates the facial animation
	* @param deltaTime The time passed since the last update
	* @returns True if succeeded, else false
	*/
	bool Update(float deltaTime);

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
	static FString GetFaceFxError();

	/**
	* Checks the FaceFX runtime execution result and the errno for errors
	* @param value The return value returned from the FaceFX call
	* @returns True if call succeeded, else false
	*/
	static bool Check(const int32& value);

	/**
	* Loads a set of animation data
	* @param animData The data to load the animation with
	* @param context The context to use
	* @returns The FaceFX handle if succeeded, else nullptr
	*/
	static struct ffx_anim_handle_t* LoadAnimation(const struct FFaceFxAnimData& animData, struct ffx_context_t* context = nullptr);

	/** The data set from where this character was loaded from */
	UPROPERTY()
	const UFaceFxActor* m_faceFxSet;

	/** The associated actor handle */
	struct ffx_actor_handle_t* m_actor_handle;

	/** The current frame state */
	struct ffx_frame_state_t* m_frame_state;

	/** The bone set handle */
	struct ffx_bone_set_handle_t* m_bone_set_handle;

	/** The handle of the currently playing animation */
	struct ffx_anim_handle_t* m_currentAnimHandle;

	/** The currently loaded xforms */
	TArray<struct ffx_bone_xform_t> m_xforms;

	/** The current bone transforms */
	TArray<FTransform> m_boneTransforms;

#if FACEFX_WITHBONEFILTER
	/** The bone transform filter indices */
	TArray<int32> m_boneTransformsFilterIndices;
#else
	/** The list of bone names defined within the FaceFX data used by this character */
	TArray<FName> m_boneNames;
#endif //FACEFX_WITHBONEFILTER

	/** The bone ids coming from the facefx asset */
	TArray<uint64> m_boneIds;

	/** The current progression */
	float m_currentTime;

	/** The total duration of the currently playing animation */
	float m_currentAnimDuration;

	/** The id of the currently played animation */
	FFaceFxAnimId m_currentAnim;

	/** Dirty indicator */
	uint8 m_isDirty : 1;

	/** Playing indicator */
	uint8 m_isPlaying : 1;

	/** Looping indicator for the currently playing animation */
	uint8 m_isLooping : 1;

#if WITH_EDITOR
	uint32 m_lastFrameNumber;
#endif //WITH_EDITOR
};