#include "FaceFX.h"
#include "Animation/FaceFXComponent.h"

UFaceFXComponent::UFaceFXComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), NumAsyncLoadRequestsPending(0)
{
}

void UFaceFXComponent::OnRegister()
{
	Super::OnRegister();

	//create characters for all entries that were setup until now
	CreateAllCharacters();
}

bool UFaceFXComponent::Setup(USkeletalMeshComponent* SkelMeshComp, const UFaceFXActor* Asset)
{
	int32 Idx = Entries.IndexOfByKey(SkelMeshComp);
	if(Idx == INDEX_NONE)
	{
		//add new entry
		Idx = Entries.Add(FFaceFXEntry(SkelMeshComp, Asset));
	}
	checkf(Idx != INDEX_NONE, TEXT("Internal Error: Unable to add new FaceFX entry."));

	if(IsRegistered())
	{
		//setup at runtime -> create character right now
		CreateCharacter(Entries[Idx]);
	}

	return false;
}

bool UFaceFXComponent::Play(FName AnimName, USkeletalMeshComponent* SkelMeshComp, bool Loop)
{
	UFaceFXCharacter* character = SkelMeshComp ? GetCharacter(SkelMeshComp) : (Entries.Num() > 0 ? Entries[0].Character : nullptr);
	if(character)
	{
		return character->Play(AnimName, NAME_None, Loop);
	}

	return false;
}

void UFaceFXComponent::OnCharacterAudioStart(class UFaceFXCharacter* Character, const struct FFaceFXAnimId& AnimId)
{
	//lookup the linked entry
	if(auto entry = Entries.FindByKey(Character))
	{
		OnPlaybackAudioStart.Broadcast(entry->SkelMeshComp, AnimId.Name);
	}
}

void UFaceFXComponent::OnCharacterPlaybackStopped(class UFaceFXCharacter* Character, const struct FFaceFXAnimId& AnimId)
{
	//lookup the linked entry
	if(auto entry = Entries.FindByKey(Character))
	{
		OnPlaybackStopped.Broadcast(entry->SkelMeshComp, AnimId.Name);
	}
}

void UFaceFXComponent::CreateAllCharacters()
{
	if(!FApp::IsGame())
	{
		//only create FaceFX characters while being in a game - not during cooking
		return;
	}

	for(FFaceFXEntry& Entry : Entries)
	{
		CreateCharacter(Entry);
	}
}

void UFaceFXComponent::CreateCharacter(FFaceFXEntry& Entry)
{
	if(Entry.Character)
	{
		//entry already contains a live character
		return;
	}

	if(!FApp::IsGame())
	{
		//only create FaceFX characters while being in a game - not during cooking
		return;
	}

	if(Entry.Asset.ToStringReference().IsValid())
	{
		if(UFaceFXActor* FaceFXActor = Entry.Asset.Get())
		{
			//initialize the FaceFX character
			Entry.Character = NewObject<UFaceFXCharacter>(this);
			checkf(Entry.Character, TEXT("Unable to instantiate a FaceFX character. Possibly Out of Memory."));

			if(!Entry.Character->Load(FaceFXActor))
			{
				UE_LOG(LogAnimation, Error, TEXT("SkeletalMesh Component FaceFX failed to get initialized. Loading failed. Component=%s. Asset=%s"), *GetName(), *Entry.Asset.ToStringReference().ToString());
				Entry.Character = nullptr;
			}
			else
			{
				//success -> register events
				Entry.Character->OnPlaybackStartAudio.AddDynamic(this, &UFaceFXComponent::OnCharacterAudioStart);
				Entry.Character->OnPlaybackStopped.AddDynamic(this, &UFaceFXComponent::OnCharacterPlaybackStopped);
			}
		}
		else
		{
			//asset not loaded yet
			++NumAsyncLoadRequestsPending;

			//asset not loaded yet -> trigger async load
			TArray<FStringAssetReference> StreamingRequests;
			StreamingRequests.Add(Entry.Asset.ToStringReference());

			FaceFX::GetStreamer().RequestAsyncLoad(StreamingRequests, FStreamableDelegate::CreateUObject(this, &UFaceFXComponent::OnFaceActorAssetLoaded));
		}
	}
	else
	{
		UE_LOG(LogAnimation, Error, TEXT("UFaceFXComponent::CreateCharacter. Internal Error: FaceFX failed to get initialized. Invalid Asset. Should not get called in this state. Component=%s"), *GetName());
	}
}

/** Async load callback for FaceFX assets */
void UFaceFXComponent::OnFaceActorAssetLoaded()
{
	//asset loaded -> create all characters which asset is now ready
	CreateAllCharacters();

	//end of async loading process
	checkf(NumAsyncLoadRequestsPending > 0);
	--NumAsyncLoadRequestsPending;
}