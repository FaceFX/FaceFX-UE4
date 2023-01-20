/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2023 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXData.h"

#include "Components/ActorComponent.h"
#include "FaceFXComponent.generated.h"

class USkeletalMeshComponent;
class UFaceFXCharacter;

/** The delegate used for various FaceFX events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFaceFXEventSignature, USkeletalMeshComponent*, SkelMeshComp, const FName&, AnimId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnFaceFXAudioStartEventSignature, USkeletalMeshComponent*, SkelMeshComp, const FName&, AnimId, bool, IsAudioStarted, UActorComponent*, AudioComponentStartedOn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnFaceFXAnimationEventSignature, USkeletalMeshComponent*, SkelMeshComp, const FName&, AnimId, int, ChannelIndex, float, ChannelTime, float, EventTime, FString, Payload);

/** A single FaceFX entry for a skelmesh */
USTRUCT(BlueprintType)
struct FACEFX_API FFaceFXEntry
{
	GENERATED_USTRUCT_BODY()

	FFaceFXEntry() : SkelMeshComp(nullptr), AudioComp(nullptr), Character(nullptr), bIsCompensateForForceFrontXAxis(false), bIsAutoPlaySound(true), bIsDisableMorphTargets(false), bIsDisableMaterialParameters(false), bIsIgnoreEvents(false) {}
	FFaceFXEntry(USkeletalMeshComponent* InSkelMeshComp, UActorComponent* InAudioComp, const TSoftObjectPtr<class UFaceFXActor>& InAsset, bool InIsCompensateForForceFrontXAxis = false, bool InIsAutoPlaySound = true, 
		bool InIsDisableMorphTargets = false, bool InbIsDisableMaterialParameters = false, bool InIsIgnoreEvents = false) :
		SkelMeshComp(InSkelMeshComp), AudioComp(InAudioComp), Asset(InAsset), Character(nullptr), bIsCompensateForForceFrontXAxis(InIsCompensateForForceFrontXAxis), bIsAutoPlaySound(InIsAutoPlaySound), 
		bIsDisableMorphTargets(InIsDisableMorphTargets), bIsDisableMaterialParameters(InbIsDisableMaterialParameters), bIsIgnoreEvents(InIsIgnoreEvents){}

	/** The linked skelmesh component */
	UPROPERTY(BlueprintReadOnly, Category=FaceFX)
	USkeletalMeshComponent* SkelMeshComp;

	/** The linked audio component */
	UPROPERTY(BlueprintReadOnly, Category=FaceFX)
	UActorComponent* AudioComp;

	/** The asset to use when instantiating the facial character instance */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=FaceFX)
	TSoftObjectPtr<class UFaceFXActor> Asset;

	/** The FaceFX character instance that was created for this component */
	UPROPERTY(Transient, BlueprintReadOnly, Category=FaceFX)
	UFaceFXCharacter* Character;

	/** Should only be enabled if Force Front XAxis was enabled when the Skeletal Mesh was imported. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=FaceFX)
	uint8 bIsCompensateForForceFrontXAxis : 1;

	/** Indicates whether or not the sound assigned to a FaceFX animation is automatically played when the animation is played. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=FaceFX, DisplayName="Automatically Play Sound")
	uint8 bIsAutoPlaySound : 1;

	/** Indicates whether or not FaceFX tracks will animate matched morph targets. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=FaceFX, DisplayName="Disable Morph Targets")
	uint8 bIsDisableMorphTargets : 1;

	/** Indicates or not FaceFX tracks will animate matched material parameters.. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = FaceFX, DisplayName="Disable Material Parameters")
	uint8 bIsDisableMaterialParameters : 1;
 
	/** Indicates whether or not FaceFX events are ignored. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = FaceFX, DisplayName="Ignore Events")
	uint8 bIsIgnoreEvents : 1;

	FORCEINLINE bool operator==(const USkeletalMeshComponent* InComp) const
	{
		return SkelMeshComp == InComp;
	}

	FORCEINLINE bool operator==(const UFaceFXCharacter* InCharacter) const
	{
		return Character == InCharacter;
	}
};

/** A component that allows to setup facial animation for skelmesh components and to use the blend facial animation nodes into their animation blueprints */
UCLASS(ClassGroup=(Rendering, Common), hidecategories=(Object, Sockets, Activation), editinlinenew, meta=(BlueprintSpawnableComponent))
class FACEFX_API UFaceFXComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	//UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~UObject

	/**
	* Sets up a FaceFX character for a given skelmesh component
	* @param SkelMeshComp The skelmesh component setting up the FaceFX character.
	* @param AudioComponent The audio component to assign to the FaceFX character. Keep empty to use the first audio component found on the owning actor.
	* @param Asset The FaceFX asset to use.
	* @param IsCompensateForForceFrontXAxis Should only be enabled if Force Front XAxis was enabled when the Skeletal Mesh was imported.
	* @param IsAutoPlaySound Indicates whether or not the sound assigned to a FaceFX animation is automatically played when the animation is played.
	* @param IsDisableMorphTargets Indicates whether or not FaceFX tracks will animate matched morph targets.
	* @param IsDisableMaterialParameters Indicates or not FaceFX tracks will animate matched material parameters.
	* @param IsIgnoreEvents Indicates whether or not FaceFX events are ignored.
	* @return True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(IsAutoPlaySound=true, HidePin="Caller", DefaultToSelf="Caller"))
	bool Setup(USkeletalMeshComponent* SkelMeshComp, 
               UActorComponent* AudioComponent,
			   const UFaceFXActor* Asset,
			   UPARAM(DisplayName="Compensate For Force Front XAxis") bool IsCompensateForForceFrontXAxis, 
			   UPARAM(DisplayName="Automatically Play Sound") bool IsAutoPlaySound,
			   UPARAM(DisplayName="Disable Morph Targets") bool IsDisableMorphTargets, 
			   UPARAM(DisplayName="Disable Material Parameters") bool IsDisableMaterialParameters, 
			   UPARAM(DisplayName="Ignore Events") bool IsIgnoreEvents, 
			   const UObject* Caller = nullptr);

	/**
	* Starts the playback of the given facial animation for a given skel mesh components character
	* @param Group The animation group
	* @param AnimName The animation to play
	* @param SkelMeshComp The skelmesh component to start the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool PlayById(FName Group, FName AnimName, USkeletalMeshComponent* SkelMeshComp = nullptr, bool Loop = false, const UObject* Caller = nullptr);

	/**
	* Starts the playback of the given facial animation for a given skel mesh components character
	* @param Animation The animation to play
	* @param SkelMeshComp The skelmesh component to start the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool Play(class UFaceFXAnim* Animation, USkeletalMeshComponent* SkelMeshComp = nullptr, bool Loop = false, const UObject* Caller = nullptr);

	/**
	* Stops the playback of the currently playing facial animation for a given skel mesh components character
	* @param SkelMeshComp The skelmesh component to stop the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool Stop(USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr);

	/**
	* Stops the playback of all currently playing facial animations for any skel mesh components character
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX)
	void StopAll();

	/**
	* Pause the playback of the currently playing facial animation for a given skel mesh components character
	* @param SkelMeshComp The skelmesh component to pause the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool Pause(USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr);

	/**
	* Resumes the playback of the currently paused facial animation for a given skel mesh components character
	* @param SkelMeshComp The skelmesh component to resume the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool Resume(USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr);

	/**
	* Jumps to a given position within the current facial animation playback or at a given animation
	* @param Position The target position to jump to (in seconds)
	* @param Pause Indicator if the playback shall be paused right afterwards
	* @param Animation The animation to start. Keep nullptr to jump within the currently playing facial animation
	* @param LoopAnimation Indicator if the animation to start shall be looped when being newly started
	* @param SkelMeshComp The skelmesh component to resume the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool JumpTo(float Position, bool Pause = false, class UFaceFXAnim* Animation = nullptr, bool LoopAnimation = false, USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr);

	/**
	* Jumps to a given position within the current facial animation playback or at a given animation
	* @param Position The target position to jump to (in seconds)
	* @param Pause Indicator if the playback shall be paused right afterwards
	* @param Animation The animation to start. Keep nullptr to jump within the currently playing facial animation
	* @param Group The animation group to start. Keep Group and AnimName to NAME_None to jump within the currently playing facial animation
	* @param AnimName The animation id to start. Keep Group and AnimName to NAME_None to jump within the currently playing facial animation
	* @param LoopAnimation Indicator if the animation to start shall be looped when being newly started
	* @param SkelMeshComp The skelmesh component to resume the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool JumpToById(float Position, bool Pause = false, FName Group = NAME_None, FName AnimName = NAME_None, bool LoopAnimation = false, USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr);

	/**
	* Gets the indicator if the facial animation playback for a given skel mesh components character is playing at the moment
	* @param SkelMeshComp The skelmesh component to check the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if playing, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool IsPlaying(USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr) const;

	/**
	* Gets the indicator if the facial animation playback for a given skel mesh components character is playing a specific animation at the moment
	* @param AnimId The id of the animation which we check playback against
	* @param SkelMeshComp The skelmesh component to check the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if playing the given animation right now, else false
	*/
	UFUNCTION(BlueprintCallable, Category = FaceFX, Meta = (HidePin = "Caller", DefaultToSelf = "Caller"))
	bool IsPlayingAnimation(const FFaceFXAnimId& AnimId, USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr) const;

	/**
	* Gets the indicator if the facial animation playback for a given skel mesh components character is paused
	* @param SkelMeshComp The skelmesh component to check the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if paused, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX, Meta=(HidePin="Caller", DefaultToSelf="Caller"))
	bool IsPaused(USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr) const;

	/**
	* Gets the indicator if the facial animation playback for a given skel mesh components character has a specific animation activated at the moment (playing or not)
	* @param AnimId The id of the animation which we check activation against
	* @param SkelMeshComp The skelmesh component to check the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @returns True if the given animation is active right now, else false
	*/
	bool IsAnimationActive(const FFaceFXAnimId& AnimId, USkeletalMeshComponent* SkelMeshComp = nullptr, const UObject* Caller = nullptr) const;

	/** Event that triggers whenever any of the FaceFX character instances plays a facial animation that requested the startup of audio playback */
	UPROPERTY(BlueprintAssignable, Category=FaceFX)
	FOnFaceFXAudioStartEventSignature OnPlaybackAudioStart;

	/** Event that triggers whenever any of the FaceFX character instances stops playing a facial animation */
	UPROPERTY(BlueprintAssignable, Category=FaceFX)
	FOnFaceFXEventSignature OnPlaybackStopped;

	/** Event that triggers whenever any of the FaceFX character instances triggers an animation event from within the FaceFX runtime */
	UPROPERTY(BlueprintAssignable, Category = FaceFX)
	FOnFaceFXAnimationEventSignature OnAnimationEvent;

	/**
	* Gets the skeletal mesh component that is targeted by the given SkelMesh identifier
	* @param SkelMeshId The skelmesh identifier
	* @returns The skelmesh component if found, else nullptr
	*/
	USkeletalMeshComponent* GetSkelMeshTarget(const struct FFaceFXSkelMeshComponentId& SkelMeshId) const;

	/**
	* Gets the FaceFX character for a given skel mesh component
	* @param SkelMeshComp The skelmesh component to get the character for. Keep empty to get the FaceFX character for the first skelmesh that was setup
	* @returns The UFaceFXCharacter for the given skelmesh context
	*/
	inline UFaceFXCharacter* GetCharacter(const USkeletalMeshComponent* SkelMeshComp = nullptr) const
	{
		if (SkelMeshComp)
		{
			if (const FFaceFXEntry* Entry = Entries.FindByKey(SkelMeshComp))
			{
				return Entry->Character;
			}
			return nullptr;
		}

		return Entries.Num() > 0 ? Entries[0].Character : nullptr;
	}

	/**
	* Gets the whole FaceFX character entry set for a given skel mesh component
	* @param SkelMeshComp The skelmesh component to get the character for. Keep empty to get the FaceFX character for the first skelmesh that was setup
	* @returns The entry set for the given skelmesh context
	*/
	inline const FFaceFXEntry* GetCharacterEntry(const USkeletalMeshComponent* SkelMeshComp = nullptr) const
	{
		if (SkelMeshComp)
		{
			if (const FFaceFXEntry* Entry = Entries.FindByKey(SkelMeshComp))
			{
				return Entry;
			}
			return nullptr;
		}

		return Entries.Num() > 0 ? &Entries[0] : nullptr;
	}

	/**
	* Gets the skel mesh component which owns a given FaceFX character instance
	* @param FaceFXCharacter The FaceFX character instance to look up the skel mesh component for
	* @returns The skel mesh component for the given FaceFX character or nullptr if not found
	*/
	inline USkeletalMeshComponent* GetSkelMeshTarget(const UFaceFXCharacter* FaceFXCharacter) const
	{
		if (FaceFXCharacter)
		{
			if (const FFaceFXEntry* Entry = Entries.FindByKey(FaceFXCharacter))
			{
				return Entry->SkelMeshComp;
			}
		}
		return nullptr;
	}

	/**
	* Gets the list of skelmesh component that are setup. Maintains the right order of setup
	* @param OutSkelMeshComponent The list of skelmesh components
	*/
	inline void GetSetupSkelMeshComponents(TArray<USkeletalMeshComponent*>& OutSkelMeshComponent) const
	{
		for (const FFaceFXEntry& Entry : Entries)
		{
			OutSkelMeshComponent.Add(Entry.SkelMeshComp);
		}
	}

	/**
	* Gets the indicator if this component currently loads at least one asset
	* @returns True if at least one asset for a character instance is pending for load, else false if all is loaded
	*/
	inline bool IsLoadingCharacterAsync() const
	{
		return NumAsyncLoadRequestsPending > 0;
	}

protected:

	//UActorComponent
	virtual void OnRegister() override;
	//~UActorComponent

private:

	/**
	* Callback for when a FaceFX character instance requested audio playback
	* @param Character The character instance who requested playback
	* @param AnimId The facial animation that is played and requested the playback
	* @param IsAudioStarted Indicator if the FaceFX character instance successfully started audio playback on its own
	* @param AudioComponentStartedOn The audio component on which the FaceFX character instance started the audio playback on. Will be set if IsAudioStarted is true
	*/
	UFUNCTION()
	void OnCharacterAudioStart(UFaceFXCharacter* Character, const FFaceFXAnimId& AnimId, bool IsAudioStarted, UActorComponent* AudioComponentStartedOn);

	/**
	* Callback for when a FaceFX character instance stopped playback
	* @param Character The character instance who stopped playback
	* @param AnimId The facial animation that was played and stopped
	*/
	UFUNCTION()
	void OnCharacterPlaybackStopped(UFaceFXCharacter* Character, const FFaceFXAnimId& AnimId);

	/**
	* Callback for when a FaceFX character instance triggers an animation event from within the FaceFX runtime
	* @param Character The character instance who triggered the event.
	* @param AnimId The facial animation that is played and triggers the event.
	* @param ChannelIndex The index of the channel that the animation is currently playing in.
	* @param ChannelTime The current playback time in the animation when the event is fired.
	* @param EventTime The exact time of playback duration at which the event was triggered.
	* @param Payload The event payload. This is a string assigned to the event directly within the FaceFX asset.
	*/
	UFUNCTION()
	void OnCharacterAnimationEvent(UFaceFXCharacter* Character, const FFaceFXAnimId& AnimId, int ChannelIndex, float ChannelTime, float EventTime, const FString& Payload);

	/** Processes the current list of registered skelmesh components and creates FaceFX characters for the ones that were not processed yet */
	void CreateAllCharacters();

	/**
	* Creates the FaceFX character for a given entry
	* @param entry The entry to create the character for
	*/
	void CreateCharacter(FFaceFXEntry& entry);

	/** Async load callback for FaceFX assets */
	void OnFaceActorAssetLoaded();

	/** The list of registered skelmesh components */
	UPROPERTY()
	TArray<FFaceFXEntry> Entries;

	/** The number of FaceFX assets that are requested for async load right now */
	uint8 NumAsyncLoadRequestsPending;
};