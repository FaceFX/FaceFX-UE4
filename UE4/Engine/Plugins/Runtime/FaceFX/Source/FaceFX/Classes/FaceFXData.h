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

#pragma once

#include "FaceFXData.generated.h"

/** The target platform for FaceFX platform specific compilation data */
namespace EFaceFXTargetPlatform
{
	enum Type
	{
		PC = 0,
		PS4,
		XBoxOne,
		MAX
	};
}

/** The struct represents a FaceFX animation identifier */
USTRUCT()
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

	/**
	* Gets the indicator if this animation ID is valid
	* @returns True if valid, else false
	*/
	inline bool IsValid() const
	{
		return Name != NAME_None;
	}

	/**
	* Resets this animation id
	*/
	inline void Reset()
	{
		Group = NAME_None;
		Name = NAME_None;
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

	/** The animation name (without the .ffxanim extension) */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FName Name;

	/** The asset file binary data */
	UPROPERTY()
	TArray<uint8> RawData;

	/**
	* Gets the indicator if this animation ID is valid
	* @returns True if valid, else false
	*/
	inline bool IsValid() const
	{
		return Name != NAME_None;
	}

	/**
	* Resets this animation id
	*/
	inline void Reset()
	{
		Name = NAME_None;
	}

	FORCEINLINE bool operator==(const FName& InName) const
	{
		return Name == InName;
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
};
