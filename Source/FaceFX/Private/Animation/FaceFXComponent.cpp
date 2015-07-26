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

bool UFaceFXComponent::Setup(USkeletalMeshComponent* SkelMeshComp, UAudioComponent* AudioComponent, const UFaceFXActor* Asset, bool IsAutoPlaySound, const UObject* Caller)
{
	if(!SkelMeshComp)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Setup. Missing SkelMeshComp argument. Caller: %s"), *GetNameSafe(Caller));
		return false;
	}

	if(!Asset)
	{
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Setup. Missing Asset argument. Caller: %s"), *GetNameSafe(Caller));
		return false;
	}

	int32 Idx = Entries.IndexOfByKey(SkelMeshComp);
	if(Idx == INDEX_NONE)
	{
		//add new entry
		Idx = Entries.Add(FFaceFXEntry(SkelMeshComp, AudioComponent, Asset, IsAutoPlaySound));
	}
	checkf(Idx != INDEX_NONE, TEXT("Internal Error: Unable to add new FaceFX entry."));

	if(IsRegistered())
	{
		//setup at runtime -> create character right now
		CreateCharacter(Entries[Idx]);
	}

	return false;
}

bool UFaceFXComponent::PlayById(FName Group, FName AnimName, USkeletalMeshComponent* SkelMeshComp, bool Loop, const UObject* Caller)
{
#if FACEFX_USEANIMATIONLINKAGE

	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->Play(AnimName, Group, Loop);
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::PlayById. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
#else
	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::PlayById. Animation linkage is disabled in the FaceFX config. Use the Play node instead and check FACEFX_USEANIMATIONLINKAGE. Caller: %s"), *GetNameSafe(Caller));
#endif //FACEFX_USEANIMATIONLINKAGE

	return false;
}

bool UFaceFXComponent::Play(UFaceFXAnim* Animation, USkeletalMeshComponent* SkelMeshComp, bool Loop, const UObject* Caller)
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->Play(Animation, Loop);
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Play. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::Stop(USkeletalMeshComponent* SkelMeshComp, const UObject* Caller)
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->Stop();
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Stop. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::Pause(USkeletalMeshComponent* SkelMeshComp, const UObject* Caller)
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->Pause();
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Pause. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::Resume(USkeletalMeshComponent* SkelMeshComp, const UObject* Caller)
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->Resume();
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::Resume. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::JumpTo(float Position, bool Pause, UFaceFXAnim* Animation, bool LoopAnimation, USkeletalMeshComponent* SkelMeshComp, const UObject* Caller)
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		if(!Character->IsPlayingOrPaused(Animation))
		{
			Character->Play(Animation, LoopAnimation);
		}

		return Character->JumpTo(Position) && (!Pause || Character->Pause());
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::JumpTo. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::JumpToById(float Position, bool Pause, FName Group, FName AnimName, bool LoopAnimation, USkeletalMeshComponent* SkelMeshComp, const UObject* Caller)
{
#if FACEFX_USEANIMATIONLINKAGE
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		const FFaceFXAnimId AnimId(Group, AnimName);

		if(!Character->IsPlayingOrPaused(AnimId) && !Character->Play(AnimId, LoopAnimation))
		{
			//new animation can't be started (most likely not found)
			Character->Stop();
			return false;
		}

		return Character->JumpTo(Position) && (!Pause || Character->Pause());
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::JumpToById. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
#else
	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::JumpToById. Animation linkage is disabled in the FaceFX config. Use the JumpTo node instead and check FACEFX_USEANIMATIONLINKAGE. Caller: %s"), *GetNameSafe(Caller));
#endif //FACEFX_USEANIMATIONLINKAGE
	return false;
}

bool UFaceFXComponent::IsPlaying(USkeletalMeshComponent* SkelMeshComp, const UObject* Caller) const
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->IsPlaying();
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::IsPlaying. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

bool UFaceFXComponent::IsPaused(USkeletalMeshComponent* SkelMeshComp, const UObject* Caller) const
{
	if(UFaceFXCharacter* Character = GetCharacter(SkelMeshComp))
	{
		return Character->IsPaused();
	}

	UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::IsPaused. FaceFX character does not exist for given SkelMeshComp <%s>. Caller: %s"), *GetNameSafe(SkelMeshComp), *GetNameSafe(Caller));
	return false;
}

USkeletalMeshComponent* UFaceFXComponent::GetSkelMeshTarget(const FFaceFXSkelMeshComponentId& SkelMeshId) const
{
	if(!SkelMeshId.IsValid())
	{
		return nullptr;
	}

	//first check for the index and matching name
	if(SkelMeshId.Index < Entries.Num())
	{
		if(USkeletalMeshComponent* SkelMeshComp = Entries[SkelMeshId.Index].SkelMeshComp)
		{
			if(SkelMeshComp->GetFName() == SkelMeshId.Name)
			{
				return SkelMeshComp;
			}
		}
	}

	//if index owns a different/renamed skelmesh component we look for the name only
	for(const FFaceFXEntry& Entry : Entries)
	{
		if(Entry.SkelMeshComp && Entry.SkelMeshComp->GetFName() == SkelMeshId.Name)
		{
			return Entry.SkelMeshComp;
		}
	}

	return nullptr;
}

void UFaceFXComponent::OnCharacterAudioStart(UFaceFXCharacter* Character, const FFaceFXAnimId& AnimId, bool IsAudioStarted, class UAudioComponent* AudioComponentStartedOn)
{
	//lookup the linked entry
	if(FFaceFXEntry* Entry = Entries.FindByKey(Character))
	{
		OnPlaybackAudioStart.Broadcast(Entry->SkelMeshComp, AnimId.Name, IsAudioStarted, AudioComponentStartedOn);
	}
}

void UFaceFXComponent::OnCharacterPlaybackStopped(UFaceFXCharacter* Character, const FFaceFXAnimId& AnimId)
{
	//lookup the linked entry
	if(auto Entry = Entries.FindByKey(Character))
	{
		OnPlaybackStopped.Broadcast(Entry->SkelMeshComp, AnimId.Name);
	}
}

void UFaceFXComponent::CreateAllCharacters()
{
	if(FApp::IsUnattended())
	{
		//only create FaceFX characters while being in a game/editor - not during cooking
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

	if(FApp::IsUnattended())
	{
		//only create FaceFX characters while being in a game/editor - not during cooking
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
				UE_LOG(LogFaceFX, Error, TEXT("SkeletalMesh Component FaceFX failed to get initialized. Loading failed. Component=%s. Asset=%s"), *GetName(), *Entry.Asset.ToStringReference().ToString());
				Entry.Character = nullptr;
			}
			else
			{
				//success -> register events
				Entry.Character->OnPlaybackStartAudio.AddDynamic(this, &UFaceFXComponent::OnCharacterAudioStart);
				Entry.Character->OnPlaybackStopped.AddDynamic(this, &UFaceFXComponent::OnCharacterPlaybackStopped);

				Entry.Character->SetAudioComponent(Entry.AudioComp);
				Entry.Character->SetAutoPlaySound(Entry.bIsAutoPlaySound);
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
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXComponent::CreateCharacter. Internal Error: FaceFX failed to get initialized. Invalid Asset. Should not get called in this state. Component=%s"), *GetName());
	}
}

/** Async load callback for FaceFX assets */
void UFaceFXComponent::OnFaceActorAssetLoaded()
{
	//asset loaded -> create all characters which asset is now ready
	CreateAllCharacters();

	//end of async loading process
	check(NumAsyncLoadRequestsPending > 0);
	--NumAsyncLoadRequestsPending;
}

void UFaceFXComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UFaceFXComponent* This = CastChecked<UFaceFXComponent>(InThis);
	
	for(FFaceFXEntry& Entry : This->Entries)
	{
		Collector.AddReferencedObject(Entry.Character);
	}
	Super::AddReferencedObjects(This, Collector);
}