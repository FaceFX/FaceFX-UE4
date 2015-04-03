#pragma once

#include "Components/ActorComponent.h"
#include "FaceFXComponent.generated.h"

/** The delegate used for various FaceFX events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFaceFXEventSignature, USkeletalMeshComponent*, SkelMeshComp, const FName&, AnimId);

/** A single FaceFX entry for a skelmesh */
USTRUCT()
struct FACEFX_API FFaceFXEntry
{
	GENERATED_USTRUCT_BODY()

	FFaceFXEntry() : SkelMeshComp(nullptr), Character(nullptr) {}
	FFaceFXEntry(class USkeletalMeshComponent* InSkelMeshComp, const TAssetPtr<class UFaceFXActor>& InAsset) : SkelMeshComp(InSkelMeshComp), Asset(InAsset), Character(nullptr) {}

	/** The linked skelmesh component */
	UPROPERTY(BlueprintReadOnly, Category=FaceFX)
	class USkeletalMeshComponent* SkelMeshComp;

	/** The asset to use when instantiating the facial character instance */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=FaceFX, meta=(DisplayName="Asset"))
	TAssetPtr<class UFaceFXActor> Asset;

	/** The FaceFX character instance that was created for this component */
	UPROPERTY(Transient, BlueprintReadOnly, Category=FaceFX, meta=(DisplayName="Character"))
	class UFaceFXCharacter* Character;

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

	/** 
	* Setups a FaceFX character to a given skelmesh component
	* @param SkelMeshComp The skelmesh component to setup the FaceFX character for
	* @param Asset The FaceFX asset to use
	* @return True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX)
	bool Setup(USkeletalMeshComponent* SkelMeshComp, const UFaceFXActor* Asset);

	/**
	* Starts the playback of the given facial animation for a given skel mesh components character
	* @param AnimName The animation to play
	* @param SkelMeshComp The skelmesh component to start the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @param Loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX)
	bool Play(FName AnimName, USkeletalMeshComponent* SkelMeshComp, bool Loop);

	/** Event that triggers whenever any of the FaceFX character instances plays a facial animation that requested the startup of audio playback */
	UPROPERTY(BlueprintAssignable, Category=FaceFX)
	FOnFaceFXEventSignature OnPlaybackAudioStart;

	/** Event that triggers whenever any of the FaceFX character instances stops playing a facial animation */
	UPROPERTY(BlueprintAssignable, Category=FaceFX)
	FOnFaceFXEventSignature OnPlaybackStopped;

	/**
	* Gets the FaceFX character instance for a given skelmesh component
	* @param skelMeshComp The skelmesh component to get the instance for
	* @return The FaceFX character instance or nullptr if not found
	*/
	inline class UFaceFXCharacter* GetCharacter(class USkeletalMeshComponent* SkelMeshComp) const
	{
		if(const FFaceFXEntry* Entry = Entries.FindByKey(SkelMeshComp))
		{
			return Entry->Character;
		}
		return nullptr;
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
	*/
	UFUNCTION()
	void OnCharacterAudioStart(class UFaceFXCharacter* Character, const struct FFaceFXAnimId& AnimId);

	/**
	* Callback for when a FaceFX character instance stopped playback
	* @param Character The character instance who stopped playback
	* @param AnimId The facial animation that was played and stopped
	*/
	UFUNCTION()
	void OnCharacterPlaybackStopped(class UFaceFXCharacter* Character, const struct FFaceFXAnimId& AnimId);

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