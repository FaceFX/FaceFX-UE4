#pragma once

#include "Components/ActorComponent.h"
#include "FaceFxComponent.generated.h"

/** A single FaceFX entry for a skelmesh */
USTRUCT()
struct FACEFX_API FFaceFxEntry
{
	GENERATED_USTRUCT_BODY()

	FFaceFxEntry() : m_skelMeshComp(nullptr), m_character(nullptr) {}
	FFaceFxEntry(class USkeletalMeshComponent* skelMeshComp, const TAssetPtr<class UFaceFxActor>& asset) : m_skelMeshComp(skelMeshComp), m_asset(asset), m_character(nullptr) {}

	/** The linked skelmesh component */
	UPROPERTY(BlueprintReadOnly, Category=FaceFx)
	class USkeletalMeshComponent* m_skelMeshComp;

	/** The asset to use when instantiating the facial character instance */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=FaceFx, meta=(DisplayName="Asset"))
	TAssetPtr<class UFaceFxActor> m_asset;

	/** The FaceFX character instance that was created for this component */
	UPROPERTY(Transient, BlueprintReadOnly, Category=FaceFx, meta=(DisplayName="Character"))
	class UFaceFxCharacter* m_character;

	FORCEINLINE bool operator==(const USkeletalMeshComponent* comp) const
	{
		return m_skelMeshComp == comp;
	}
};

/** A component that allows to setup facial animation for skelmesh components and to use the blend facial animation nodes into their animation blueprints */
UCLASS(ClassGroup=(Rendering, Common), hidecategories=(Object, Sockets, Activation), editinlinenew, meta=(BlueprintSpawnableComponent))
class FACEFX_API UFaceFxComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** 
	* Setups a FaceFX character to a given skelmesh component
	* @param skelMeshComp The skelmesh component to setup the FaceFX character for
	* @param asset The FaceFX asset to use
	* @return True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX)
	bool Setup(USkeletalMeshComponent* skelMeshComp, const UFaceFxActor* asset);

	/**
	* Starts the playback of the given facial animation for a given skel mesh components character
	* @param animName The animation to play
	* @param skelMeshComp The skelmesh component to start the playback for. Keep nullptr to use the first setup skelmesh component character instead
	* @param loop True for when the animation shall loop, else false
	* @returns True if succeeded, else false
	*/
	UFUNCTION(BlueprintCallable, Category=FaceFX)
	bool Play(FName animName, USkeletalMeshComponent* skelMeshComp = nullptr, bool loop = false);

	/**
	* Gets the FaceFX character instance for a given skelmesh component
	* @param skelMeshComp The skelmesh component to get the instance for
	* @return The FaceFX character instance or nullptr if not found
	*/
	inline class UFaceFxCharacter* GetCharacter(class USkeletalMeshComponent* skelMeshComp) const
	{
		if(const FFaceFxEntry* entry = m_entries.FindByKey(skelMeshComp))
		{
			return entry->m_character;
		}
		return nullptr;
	}

	/**
	* Gets the indicator if this component currently loads at least one asset
	*/
	inline bool IsLoadingFaceFxCharacterAsync() const
	{
		return m_faceFxCharacterIsLoadingAsync > 0;
	}

protected:

	//UActorComponent
	virtual void OnRegister() override;
	//~UActorComponent

private:

	/** Processes the current list of registered skelmesh components and creates FaceFX characters for the ones that were not processed yet */
	void CreateAllFaceFxCharacters();

	/** 
	* Creates the FaceFX character for a given entry
	* @param entry The entry to create the character for
	*/
	void CreateFaceFxCharacter(FFaceFxEntry& entry);

	/** Async load callback for FaceFX assets */
	void OnFaceFxAssetLoaded();

	/** The list of registered skelmesh components */
	UPROPERTY()
	TArray<FFaceFxEntry> m_entries;

	/** Indicator if the FaceFX assets are loaded up right now */
	uint8 m_faceFxCharacterIsLoadingAsync;
};