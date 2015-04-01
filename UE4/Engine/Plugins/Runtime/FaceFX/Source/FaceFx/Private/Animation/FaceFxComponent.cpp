#include "FaceFx.h"
#include "Animation/FaceFxComponent.h"

UFaceFxComponent::UFaceFxComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), m_faceFxCharacterIsLoadingAsync(0)
{
}

void UFaceFxComponent::OnRegister()
{
	Super::OnRegister();

	//create characters for all entries that were setup until now
	CreateAllFaceFxCharacters();
}

/** 
* Setups a FaceFX character to a given skelmesh component
* @param skelMeshComp The skelmesh component to setup the FaceFX character for
* @param asset The FaceFX asset to use
* @return True if succeeded, else false
*/
bool UFaceFxComponent::Setup(USkeletalMeshComponent* skelMeshComp, const UFaceFxActor* asset)
{
	int32 idx = m_entries.IndexOfByKey(skelMeshComp);
	if(idx == INDEX_NONE)
	{
		//add new entry
		idx = m_entries.Add(FFaceFxEntry(skelMeshComp, asset));
	}
	checkf(idx != INDEX_NONE, TEXT("Internal Error: Unable to add new FaceFX entry."));

	if(IsRegistered())
	{
		//setup at runtime -> create character right now
		CreateFaceFxCharacter(m_entries[idx]);
	}

	return false;
}

/**
* Starts the playback of the given facial animation for a given skel mesh components character
* @param animName The animation to play
* @param skelMeshComp The skelmesh component to start the playback for. Keep nullptr to use the first setup skelmesh component character instead
* @param loop True for when the animation shall loop, else false
* @returns True if succeeded, else false
*/
bool UFaceFxComponent::Play(FName animName, USkeletalMeshComponent* skelMeshComp, bool loop)
{
	UFaceFxCharacter* character = skelMeshComp ? GetCharacter(skelMeshComp) : (m_entries.Num() > 0 ? m_entries[0].m_character : nullptr);
	if(character)
	{
		return character->Play(animName, NAME_None, loop);
	}
	return false;
}

/** Processes the current list of registered skelmesh components and creates FaceFX characters for the ones that were not processed yet */
void UFaceFxComponent::CreateAllFaceFxCharacters()
{
	if(!FApp::IsGame())
	{
		//only create FaceFX characters while being in a game - not during cooking
		return;
	}

	for(FFaceFxEntry& entry : m_entries)
	{
		CreateFaceFxCharacter(entry);
	}
}

/** 
* Creates the FaceFX character for a given entry
* @param entry The entry to create the character for
*/
void UFaceFxComponent::CreateFaceFxCharacter(FFaceFxEntry& entry)
{
	if(entry.m_character)
	{
		//entry already contains a live character
		return;
	}

	if(!FApp::IsGame())
	{
		//only create FaceFX characters while being in a game - not during cooking
		return;
	}

	if(entry.m_asset.ToStringReference().IsValid())
	{
		if(UFaceFxActor* fxActor = entry.m_asset.Get())
		{
			//initialize the FaceFX character
			entry.m_character = NewObject<UFaceFxCharacter>(this);
			checkf(entry.m_character, TEXT("Unable to instantiate a FaceFX character. Possibly Out of Memory."));

			if(!entry.m_character->Load(fxActor))
			{
				UE_LOG(LogAnimation, Error, TEXT("SkeletalMesh Component FaceFX failed to get initialized. Loading failed. Component=%s. Asset=%s"), *GetName(), *entry.m_asset.ToStringReference().ToString());
				entry.m_character = nullptr;
			}
		}
		else
		{
			//asset not loaded yet
			++m_faceFxCharacterIsLoadingAsync;

			//asset not loaded yet -> trigger async load
			TArray<FStringAssetReference> streamingRequests;
			streamingRequests.Add(entry.m_asset.ToStringReference());

			FaceFx::GetStreamer().RequestAsyncLoad(streamingRequests, FStreamableDelegate::CreateUObject(this, &UFaceFxComponent::OnFaceFxAssetLoaded));
		}
	}
	else
	{
		UE_LOG(LogAnimation, Error, TEXT("UFaceFxComponent::CreateFaceFxCharacter. Internal Error: FaceFX failed to get initialized. Invalid Asset. Should not get called in this state. Component=%s"), *GetName());
	}
}

/** Async load callback for FaceFX assets */
void UFaceFxComponent::OnFaceFxAssetLoaded()
{
	//asset loaded -> create all characters which asset is now ready
	CreateAllFaceFxCharacters();

	//end of async loading process
	checkf(m_faceFxCharacterIsLoadingAsync > 0);
	--m_faceFxCharacterIsLoadingAsync;
}