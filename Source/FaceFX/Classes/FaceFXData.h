/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2020 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXConfig.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FaceFXData.generated.h"

/** The different playback states */
enum class EPlaybackState : uint8
{
	Playing,
	Paused,
	Stopped
};

/** The struct represents a FaceFX animation identifier */
USTRUCT(BlueprintType)
struct FFaceFXAnimId
{
	GENERATED_USTRUCT_BODY()

	FFaceFXAnimId(const FName& InGroup = NAME_None, const FName& InName = NAME_None) : Group(InGroup), Name(InName) {}

	/** The animation group */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName Group;

	/** The animation name (without the .ffxanim extension) */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName Name;

	inline bool IsValid() const
	{
		return Name != NAME_None;
	}

	inline void Reset()
	{
		Group = NAME_None;
		Name = NAME_None;
	}

	/**
	* Generates a <group.animation> id
	* @returns The <group.animation> id
	*/
	FString GetIdString() const
	{
		FString IdString = Group.ToString() + TEXT(".") + Name.ToString();
		return MoveTemp(IdString);
	}

	/**
	* Parses the group and animation id from a <group.animation> id
	* @param Id The <group.animation> id to parse from
	* @param OutGroup The resulting group name
	* @param OutAnim The resulting animation namec
	* @returns True if succeeded, else false
	*/
	static bool ParseIdString(const FString& Id, FString& OutGroup, FString& OutAnim)
	{
		return Id.Split(TEXT("."), &OutGroup, &OutAnim, ESearchCase::CaseSensitive);
	}

	/**
	* Sets the group and animation id from a <group.animation> id
	* @param Id The <group.animation> id to parse from
	* @returns True if succeeded, else false
	*/
	bool SetFromIdString(const FString& Id)
	{
		FString GroupS, IdS;
		if(ParseIdString(Id, GroupS, IdS))
		{
			Name = FName(*IdS);
			Group = FName(*GroupS);
			return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(const FFaceFXAnimId& id) const
	{
		return id.Group == Group && id.Name == Name;
	}

	FORCEINLINE bool operator!=(const FFaceFXAnimId& id) const
	{
		return !operator==(id);
	}
};

/** The struct that holds the data for a single FaceFX animation */
USTRUCT()
struct FFaceFXAnimData
{
	GENERATED_USTRUCT_BODY()

	/** The asset file binary data for the .ffxanim file */
	UPROPERTY()
	TArray<uint8> RawData;

	inline bool IsValid() const
	{
		return RawData.Num() > 0;
	}

	inline void Reset()
	{
		RawData.Empty();
	}
};

/** The struct that holds the FaceFX id data which is a mapping from an numeric ID to a string */
USTRUCT()
struct FFaceFXIdData
{
	GENERATED_USTRUCT_BODY()

	FFaceFXIdData(uint64 InId = 0, FName InName = NAME_None) : Id(InId), Name(InName) {}

	/** The numeric id */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	uint64 Id;

	/** The corresponding name */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName Name;

	FORCEINLINE bool operator==(const uint64& InId) const
	{
		return Id == InId;
	}

	FORCEINLINE bool operator==(const FName& InName) const
	{
		return Name == InName;
	}

	FORCEINLINE bool operator==(const FString& InName) const
	{
		return Name.GetPlainNameString().Equals(InName);
	}
};

/** A single FaceFX Animation set entry used to differentiate per platform */
USTRUCT()
struct FFaceFXAnimSetData
{
	GENERATED_USTRUCT_BODY()

	/** The animation data */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFXAnimData> Animations;
};

/** A single FaceFX actor data entry used to differentiate per platform */
USTRUCT()
struct FFaceFXActorData
{
	GENERATED_USTRUCT_BODY()

	/** The asset file binary data for the .ffxactor file */
	UPROPERTY()
	TArray<uint8> ActorRawData;

	/** The asset file binary data for the .ffxbones file */
	UPROPERTY()
	TArray<uint8> BonesRawData;

	/** The id/name mappings */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<FFaceFXIdData> Ids;

	inline bool IsValid() const
	{
		// Note: for a morph-only character there may not be any bones data, but there
		// must be actor data and ids for the data to be considered valid.
		return ActorRawData.Num() > 0 && Ids.Num() > 0;
	}

	inline void Reset()
	{
		ActorRawData.Empty();
		BonesRawData.Empty();
		Ids.Empty();
	}
};

/** A struct that represents a skel mesh component on a FaceFX Component */
USTRUCT()
struct FFaceFXSkelMeshComponentId
{
	GENERATED_USTRUCT_BODY()

	FFaceFXSkelMeshComponentId() : Index(INDEX_NONE) {}

	/** The index of the setup within a FaceFX component */
	UPROPERTY()
	int32 Index;

	/** The expected name of the skel mesh component */
	UPROPERTY()
	FName Name;

	inline bool IsValid() const
	{
		return Index != INDEX_NONE && !Name.IsNone();
	}

	inline void Reset()
	{
		Index = INDEX_NONE;
		Name = NAME_None;
	}

	FORCEINLINE bool operator==(const FFaceFXSkelMeshComponentId& Id) const
	{
		return Index == Id.Index && Name == Id.Name;
	}
};
