#include "FaceFx.h"
#include "FaceFxContext.h"

DECLARE_STATS_GROUP(TEXT("FaceFX"),STATGROUP_FACEFX, STATCAT_Advance);
DECLARE_CYCLE_STAT(TEXT("Tick FaceFX Character"), STAT_FaceFXTick, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Update FaceFX Transforms"), STAT_FaceFXUpdateTransforms, STATGROUP_FACEFX);
DECLARE_CYCLE_STAT(TEXT("Load FaceFX Assets"), STAT_FaceFXLoad, STATGROUP_FACEFX);

UFaceFxCharacter::UFaceFxCharacter(const class FObjectInitializer& PCIP) : Super(PCIP),
	m_actor_handle(nullptr),
	m_frame_state(nullptr),
	m_bone_set_handle(nullptr),
	m_currentAnimHandle(nullptr),
	m_currentTime(.0F),
	m_currentAnimDuration(.0F),
	m_isDirty(true),
	m_isPlaying(false),
	m_isLooping(false)
#if WITH_EDITOR
	,m_lastFrameNumber(0)
#endif
{
}

UFaceFxCharacter::~UFaceFxCharacter()
{
	Reset();
}

void UFaceFxCharacter::Tick(float deltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FaceFXTick);

#if WITH_EDITOR
	if(m_lastFrameNumber == GFrameNumber)
	{
		//tick was called twice within the same frame -> happens when running in PIE with dedicated server active or multiple instances
		return;
	}
	m_lastFrameNumber = GFrameNumber;
#endif

	//progress in time
	m_currentTime += deltaTime;

	if(m_currentTime >= m_currentAnimDuration)
	{
		//end of animation reached
		if(IsLooping())
		{
			Restart();
		}
		else
		{
			Stop();
		}
	}

	if (!Check(ffx_process_frame(m_actor_handle, m_frame_state, m_currentTime)))
	{
		//update failed
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Tick. Update failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return;
	}

	m_isDirty = true;
}

bool UFaceFxCharacter::IsTickable() const
{
	return IsPlaying();
}

TStatId UFaceFxCharacter::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFaceFxCharacter, STATGROUP_Tickables);
}

/** 
* Gets the start and end time of the current animation
* @param outStart The start time if call succeeded
* @param outEnd The end time if call succeeded
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::GetAnimationBounds(float& outStart, float& outEnd) const
{
	if(!IsPlaying() || !m_currentAnimHandle)
	{
		return false;
	}

	//get anim bounds
	if(!Check(ffx_get_anim_bounds(m_currentAnimHandle, &outStart, &outEnd)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::GetAnimationBounds. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	return true;
}

/**
* Starts the playback of the given facial animation
* @param animId The animation to play
* @param loop True for when the animation shall loop, else false
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::Play(const FFaceFxAnimId& animId, bool loop)
{
	if(!m_faceFxSet)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. FaceFX asset not loaded."));
		return false;
	}

	if(!animId.IsValid())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. Invalid animation name/group. Group=%s, Anim=%s. Asset: %s"), *animId.m_group.GetPlainNameString(), *animId.m_name.GetPlainNameString(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if(!IsLoaded())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. FaceFX character not loaded. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if(IsPlaying())
	{
		UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::Play. Animation already playing. Asset: %s"), *GetNameSafe(m_faceFxSet));
		Stop();
	}

	const FFaceFxAnimData* anim = m_faceFxSet->GetAnimation(animId);
	if(!anim)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. FaceFX animation not found. Group=%s, Anim=%s. Asset: %s"), *animId.m_group.GetPlainNameString(), *animId.m_name.GetPlainNameString(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if(m_currentAnim != anim->m_name)
	{
		//animation changed -> create new handle

		//load animation
		UnloadCurrentAnim();
		m_currentAnimHandle = LoadAnimation(*anim);

		if(!m_currentAnimHandle)
		{
			//errors given within LoadAnimation
			return false;
		}
	}

	//get anim bounds
	float animStart = .0F;
	float animEnd = .0F;
	if(!Check(ffx_get_anim_bounds(m_currentAnimHandle, &animStart, &animEnd)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//sanity check
	m_currentAnimDuration = animEnd - animStart;
	if(m_currentAnimDuration <= .0F)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. Invalid animation length of %.3f seconds. Asset: %s"), m_currentAnimDuration, *GetNameSafe(m_faceFxSet));
		return false;
	}

	size_t assigned_channel = 0;
	if(!Check(ffx_play(m_actor_handle, m_currentAnimHandle, &assigned_channel)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Play. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//reset timer to the beginning of the animation
	m_currentTime = animStart;
	m_currentAnim = animId;
	m_isPlaying = true;
	m_isLooping = loop;

	return true;
}

/**
* Resumes the playback of the facial animation
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::Resume()
{
	if(!IsLoaded())
	{
		UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::Resume. FaceFX character not loaded. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if(IsPlaying())
	{
		UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::Resume. Animation still playing. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if(!Check(ffx_resume(m_actor_handle, m_currentTime)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Resume. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}
	
	m_isPlaying = true;

	return true;
}

/**
* Pauses the playback of the facial animation
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::Pause()
{
	if(!IsLoaded())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Pause. FaceFX character not loaded. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	if (IsPlaying() && !Check(ffx_pause(m_actor_handle, m_currentTime)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Pause. FaceFX call failed. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	m_isPlaying = false;

	return true;
}

/**
* Stops the playback of this facial animation
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::Stop()
{
	if(!IsLoaded())
	{
		//not loaded yet
		return false;
	}

	if (IsPlaying() && !Check(ffx_stop(m_actor_handle)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Stop. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//reset timer
	m_currentTime = .0F;
	m_currentAnim.Reset();
	m_isPlaying = false;

	UnloadCurrentAnim();

	return true;
}

/**
* Restarts the current animation
* @returns True if succeeded, else false
*/
bool UFaceFxCharacter::Restart()
{
	if(!IsLoaded())
	{
		return false;
	}

	if(!m_currentAnimHandle)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Restart. Current animation is invalid. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	float startTime, endTime;
	if(!GetAnimationBounds(startTime, endTime))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Restart. Fetching animation bounds failed. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//we need to explicitly stop and start the playback again. Just resetting the current time is not enough

	//stop
	if (!Check(ffx_stop(m_actor_handle)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Restart. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//play again
	size_t assigned_channel = 0;
	if(!Check(ffx_play(m_actor_handle, m_currentAnimHandle, &assigned_channel)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Restart. FaceFX call failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return false;
	}

	//reset timer
	m_currentTime = startTime;

	return true;
}

/** Reset the whole character setup */
void UFaceFxCharacter::Reset()
{
	//Stop any playing animation before destroying the handles
	Stop();

	//free the facefx handles
	UnloadCurrentAnim();

	if (m_actor_handle) 
	{
		ffx_destroy_actor_handle(&m_actor_handle, nullptr, nullptr);
		m_actor_handle = nullptr;
	}

	if(m_frame_state)
	{
		ffx_destroy_frame_state(&m_frame_state);
		m_frame_state = nullptr;
	}

	if (m_bone_set_handle)
	{
		ffx_destroy_bone_set_handle(&m_bone_set_handle, nullptr, nullptr);
		m_bone_set_handle = nullptr;
	}

	//reset arrays
	m_xforms.Empty();
	m_boneTransforms.Empty();
	m_boneIds.Empty();

	m_faceFxSet = nullptr;

	m_isDirty = true;
}

/**
* Loads the character data from the given data set
* @param dataset The data set to load from
* @param boneFilter The bone filter to apply. Keep nullptr to apply the whole bone set as a filter
* @returns True if succeeded, else false
*/
#if FACEFX_WITHBONEFILTER
bool UFaceFxCharacter::Load(const UFaceFxActor* dataset, const TArray<FName>* boneFilter)
#else
bool UFaceFxCharacter::Load(const UFaceFxActor* dataset)
#endif //Y_FACEFX_WITHBONEFILTER
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXLoad);

	if(!dataset)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Missing dataset."));
		return false;
	}

	if(!dataset->IsValid())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Invalid dataset."));
		return false;
	}

	Reset();

	m_faceFxSet = dataset;

	const FFaceFxActorData& data = m_faceFxSet->GetData();

	//  We chose a global context for this sample.
	ffx_context_t context = FFaceFxContext::GetInstance().CreateFaceFxContext();

	if (!Check(ffx_create_bone_set_handle((char*)(&data.m_bonesRawData[0]), data.m_bonesRawData.Num(), FFX_RUN_INTEGRITY_CHECK, FFX_USE_FULL_XFORMS, &m_bone_set_handle, &context))) 
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to create FaceFX bone handle. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		Reset();
		return false;
	}

	static size_t channel_count  = 4;
	if (!Check(ffx_create_actor_handle((char*)(&data.m_actorRawData[0]), data.m_actorRawData.Num(), FFX_RUN_INTEGRITY_CHECK, channel_count, &m_actor_handle, &context)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to create FaceFX actor handle. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		Reset();
		return false;
	}

	if (!Check(ffx_create_frame_state(m_actor_handle, &m_frame_state, &context)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to create FaceFX state. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		Reset();
		return false;
	}


	size_t track_count = 0;
	if (!Check(ffx_get_actor_track_count(m_actor_handle, &track_count)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to receive FaceFX track count. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		Reset();
		return false;
	}

	size_t xform_count = 0;
	if (!Check(ffx_get_bone_set_bone_count(m_bone_set_handle, &xform_count)))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to receive FaceFX bone count. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		Reset();
		return false;
	}


	if(xform_count > 0)
	{
		//prepare buffer
		m_xforms.AddUninitialized(xform_count);
		
		//retrieve bone names
		m_boneIds.AddUninitialized(xform_count);

		if(!Check(ffx_get_bone_set_bone_ids(m_bone_set_handle, reinterpret_cast<uint64_t*>(m_boneIds.GetData()), xform_count)))
		{
			UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::Load. Unable to receive FaceFX bone names. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
			Reset();
			return false;
		}

#if FACEFX_WITHBONEFILTER
		if(boneFilter)
		{
			SetBoneFilter(*boneFilter);
		}
		else
#endif //FACEFX_WITHBONEFILTER
		{
			//apply default bone filter using ALL bones from the facefx id list

#if FACEFX_WITHBONEFILTER
			TArray<FName> names;
#else
			//prepare transform buffer
			m_boneTransforms.AddZeroed(xform_count);
			
			TArray<FName>& names = m_boneNames;
#endif //FACEFX_WITHBONEFILTER

			//match bone ids with names from the .ffxids assets
			for(const uint64& id : m_boneIds)
			{
				if(const FFaceFxIdData* boneId = data.m_ids.FindByKey(id))
				{
					names.Add(boneId->m_name);
				}
				else
				{
					UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::Load. Unknown bone id. %i. Asset: %s"), id, *GetNameSafe(m_faceFxSet));
				}
			}

#if FACEFX_WITHBONEFILTER
			//apply bone filter -> filter none as default
			SetBoneFilter(names);
#endif //Y_FACEFX_WITHBONEFILTER
		}
	}

	return true;
}

#if FACEFX_WITHBONEFILTER

/**
* Sets the bones we want to get the transforms for within GetBoneTransforms. The order will be defined by this
* @param bonesNames The names of the bones to load
* @returns True if succeeded, else false (i.e. when the character have not been loaded yet)
*/
bool UFaceFxCharacter::SetBoneFilter(const TArray<FName>& bonesNames)
{
	if(!IsLoaded())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::SetBoneFilter. Character not loaded yet. Asset: %s"), *GetNameSafe(m_faceFxSet));
		return false;
	}

	m_boneTransformsFilterIndices.Empty();

	const FFaceFxActorData& data = m_faceFxSet->GetData();

	for(const FName& boneName : bonesNames)
	{
		if(boneName == NAME_None)
		{
			continue;
		}

		//lookup bone id
		//retrieve bone names
		if(const FFaceFxIdData* boneId = data.m_ids.FindByKey(boneName))
		{
			//locate bone id
			const int32 idx = m_boneIds.Find(boneId->m_id);
			if(idx != INDEX_NONE)
			{
				m_boneTransformsFilterIndices.Add(idx);
			}
			else
			{
				UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::SetBoneFilter. Unknown bone id: %i (%s). This bone is not part of the FaceFX bone setup. Asset: %s"), boneId->m_id, *boneName.GetPlainNameString(), *GetNameSafe(m_faceFxSet));
			}			
		}
		else
		{
			UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::SetBoneFilter. Bone not defined within the FaceFX id mapping list: %s. Asset: %s"), *boneName.GetPlainNameString(), *GetNameSafe(m_faceFxSet));
		}
	}

	//prepare transforms buffer
	m_boneTransforms.AddZeroed(m_boneTransformsFilterIndices.Num());

	return true;
}

/**
* Gets the names of the bones that are currently filtered
* @param outResult The resulting bone names
*/
void UFaceFxCharacter::GetFilteredBoneNames(TArray<FName>& outResult) const
{
	if(!m_faceFxSet.IsValid())
	{
		//not loaded yet
		return;
	}

	const FFaceFxActorData& data = m_faceFxSet->GetData();

	for(const int32& idx : m_boneTransformsFilterIndices)
	{
		if(const FFaceFxIdData* boneId = idx < m_boneIds.Num() ? data.m_ids.FindByKey(m_boneIds[idx]) : nullptr)
		{
			outResult.Add(boneId->m_name);
		}
	}
}

#endif //Y_FACEFX_WITHBONEFILTER

/**
* Gets the index within the transforms for a given bone name
* @param name The bone name to look for
* @returns The transforms index, INDEX_NONE if not found
*/
int32 UFaceFxCharacter::GetBoneNameTransformIndex(const FName& name) const
{
	if(const FFaceFxIdData* boneId = m_faceFxSet ? m_faceFxSet->GetData().m_ids.FindByKey(name) : nullptr)
	{
		//get index of the FaceFX bone
		int32 faceFxBoneIdx;
		if(m_boneIds.Find(boneId->m_id, faceFxBoneIdx))
		{
#if FACEFX_WITHBONEFILTER
			int32 result;
			if(m_boneTransformsFilterIndices.Find(faceFxBoneIdx, result))
			{
				return result;
			}
#else
			return faceFxBoneIdx;
#endif //Y_FACEFX_WITHBONEFILTER
		}
	}
	return INDEX_NONE;
}

/**
* Unload the current animation
*/
void UFaceFxCharacter::UnloadCurrentAnim()
{
	if (m_currentAnimHandle)
	{
		ffx_destroy_anim_handle(&m_currentAnimHandle, nullptr, nullptr);
		m_currentAnimHandle = nullptr;
	}
}

/**
* Loads a set of animation data
* @param animData The data to load the animation with
* @param context The optional context to use
* @returns The FaceFX handle if succeeded, else nullptr
*/
struct ffx_anim_handle_t* UFaceFxCharacter::LoadAnimation(const FFaceFxAnimData& animData, struct ffx_context_t* context)
{
	//load animations
	if(animData.m_rawData.Num() == 0)
	{
		UE_LOG(LogFaceFx, Warning, TEXT("UFaceFxCharacter::LoadAnimation. Missing facefx animation data for <%s>."), *(animData.m_name.GetPlainNameString()));
		return nullptr;
	}

	ffx_context_t ctx = context ? *context : FFaceFxContext::GetInstance().CreateFaceFxContext();

	ffx_anim_handle_t* newHandle = nullptr;
	if (!Check(ffx_create_anim_handle((char*)(&animData.m_rawData[0]), animData.m_rawData.Num(), FFX_RUN_INTEGRITY_CHECK, &newHandle, &ctx)) || !newHandle)
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::LoadAnimation. Unable to create animation FaceFX handle. %s"), *GetFaceFxError());
	}
	
	return newHandle;
}

/** Updates the local bone transform table */
void UFaceFxCharacter::UpdateTransforms()
{
	SCOPE_CYCLE_COUNTER(STAT_FaceFXUpdateTransforms);
	
	if(m_xforms.Num() == 0)
	{
		//no buffer created yet
		return;
	}

#if FACEFX_WITHBONEFILTER
	if(m_boneTransformsFilterIndices.Num() == 0)
	{
		//all filtered out. Nothing to update
		m_isDirty = false;
		return;
	}
#endif //Y_FACEFX_WITHBONEFILTER

	if(!Check(ffx_calc_frame_bone_xforms(m_bone_set_handle, m_frame_state, &m_xforms[0], m_xforms.Num())))
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxCharacter::UpdateTransforms. Calculating bone transforms failed. %s. Asset: %s"), *GetFaceFxError(), *GetNameSafe(m_faceFxSet));
		return;
	}

	//fill transform buffer
#if FACEFX_WITHBONEFILTER
	for(int32 i=0; i<m_boneTransformsFilterIndices.Num(); ++i)
	{
		const ffx_bone_xform_t& xform = m_xforms[m_boneTransformsFilterIndices[i]];
#else
	for(int32 i=0; i<m_xforms.Num(); ++i)
	{
		const ffx_bone_xform_t& xform = m_xforms[i];
#endif //Y_FACEFX_WITHBONEFILTER

		//the coordinate system of bones is just like the old FaceFX in UE3: native for the animation package you used to create the actor, with the w component of the quaternion negated
		//All transforms are in parent space.
		//We also need to bring form FaceFX to UE space by reverting rotation.z and translation.y

		m_boneTransforms[i].SetComponents(
			//w, x, y, z quaternion rotation
			FQuat(xform.rot[1], -xform.rot[2], xform.rot[3], xform.rot[0]),
			//x, y, z position
			FVector(xform.pos[0], -xform.pos[1], xform.pos[2]), 
			//x, y, z scale
			FVector(xform.scl[0], -xform.scl[1], xform.scl[2]));
	}

	m_isDirty = false;
}

/**
* Gets the latest internal facefx error message
* @returns The last error message
*/
FString UFaceFxCharacter::GetFaceFxError()
{
	char msg[128] = {0};
	if (ffx_strerror(ffx_errno(), msg, 128)) 
	{
		return FString(msg);
	} 
	else 
	{
		return TEXT("Unable to retrieve additional FaceFX error message.");
	}
}

/**
* Checks the FaceFX runtime execution result and the errno for errors
* @param value The return value returned from the FaceFX call
* @returns True if call succeeded, else false
*/
bool UFaceFxCharacter::Check(const int32& value)
{
	return value != 0 && (ffx_errno() == EOK);
}
