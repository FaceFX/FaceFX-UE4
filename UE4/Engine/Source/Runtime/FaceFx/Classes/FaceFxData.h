#pragma once

#include "FaceFxData.generated.h"

/** The struct represents a FaceFX animation identifier */
USTRUCT()
struct FFaceFxAnimId
{
	GENERATED_USTRUCT_BODY()

	FFaceFxAnimId(const FName& group = NAME_None, const FName& name = NAME_None) : m_group(group), m_name(name) {}

	/** The animation group */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName m_group;

	/** The animation name (without the .ffxanim extension) */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName m_name;

	/**
	* Gets the indicator if this animation ID is valid
	* @returns True if valid, else false
	*/
	inline bool IsValid() const
	{
		return m_name != NAME_None;
	}

	/**
	* Resets this animation id
	*/
	inline void Reset()
	{
		m_group = NAME_None;
		m_name = NAME_None;
	}

	FORCEINLINE bool operator==(const FFaceFxAnimId& id) const
	{
		return id.m_group == m_group && id.m_name == m_name;
	}

	FORCEINLINE bool operator!=(const FFaceFxAnimId& id) const
	{
		return !operator==(id);
	}
};

/** The struct that holds the data for a single FaceFX animation */
USTRUCT()
struct FFaceFxAnimData
{
	GENERATED_USTRUCT_BODY()

	/** The animation name (without the .ffxanim extension) */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName m_name;

	/** The asset file binary data */
	UPROPERTY()
	TArray<uint8> m_rawData;

	/**
	* Gets the indicator if this animation ID is valid
	* @returns True if valid, else false
	*/
	inline bool IsValid() const
	{
		return m_name != NAME_None;
	}

	/**
	* Resets this animation id
	*/
	inline void Reset()
	{
		m_name = NAME_None;
	}

	FORCEINLINE bool operator==(const FName& name) const
	{
		return m_name == name;
	}
};

/** The struct that holds the FaceFX id data which is a mapping from an numeric ID to a string */
USTRUCT()
struct FFaceFxIdData
{
	GENERATED_USTRUCT_BODY()

	FFaceFxIdData(uint64 id = 0, FName name = NAME_None) : m_id(id), m_name(name) {}

	/** The numeric id */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	uint64 m_id;

	/** The corresponding name */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName m_name;

	FORCEINLINE bool operator==(const uint64& id) const
	{
		return m_id == id;
	}

	FORCEINLINE bool operator==(const FName& name) const
	{
		return m_name == name;
	}

	FORCEINLINE bool operator==(const FString& name) const
	{
		return m_name.GetPlainNameString().Equals(name);
	}
};

/** A single FaceFX Animation set entry used to differentiate per platform */
USTRUCT()
struct FFaceFxAnimSetData
{
	GENERATED_USTRUCT_BODY()

	/** The animation data */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFxAnimData> m_animations;
};

/** A single FaceFX actor data entry used to differentiate per platform */
USTRUCT()
struct FFaceFxActorData
{
	GENERATED_USTRUCT_BODY()

	/** The asset file binary data for the .ffxactor file */
	UPROPERTY()
	TArray<uint8> m_actorRawData;

	/** The asset file binary data for the .ffxbones file */
	UPROPERTY()
	TArray<uint8> m_bonesRawData;

	/** The id/name mappings */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFxIdData> m_ids;
};