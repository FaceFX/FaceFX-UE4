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

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXActor::UFaceFXActor(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

SIZE_T UFaceFXActor::GetResourceSize(EResourceSizeMode::Type Mode)
{
	SIZE_T ResSize = Super::GetResourceSize(Mode);

	//only count cooked data without any references
	if(Mode == EResourceSizeMode::Exclusive)
	{
		if(PlatformData.Num() > 0)
		{
			//take the first entry as an approximation
			const FFaceFXActorData& Data = PlatformData[0];
			ResSize += Data.ActorRawData.Num() * Data.ActorRawData.GetTypeSize();
			ResSize += Data.BonesRawData.Num() * Data.BonesRawData.GetTypeSize();
			ResSize += Data.Ids.Num() * Data.Ids.GetTypeSize();
		}
	}
	else
	{
#if FACEFX_USEANIMATIONLINKAGE
		for(UFaceFXAnim* Anim : Animations)
		{
			if(Anim)
			{
				ResSize += Anim->GetResourceSize(Mode);
			}
		}
#endif
	}

	return ResSize;
}

#if WITH_EDITORONLY_DATA

void UFaceFXActor::Serialize(FArchive& Ar)
{
	if(!IsTemplate() && Ar.IsSaving())
	{
		if(Ar.IsCooking())
		{
			ClearPlatformData(Ar, PlatformData);
		}

#if FACEFX_USEANIMATIONLINKAGE
		//cleanup references to deleted assets
		for(int32 i=Animations.Num()-1; i>=0; --i)
		{
			if(!Animations[i])
			{
				Animations.RemoveAt(i);
			}
		}
#else
		//clean all animation data
		Animations.Empty();
#endif //FACEFX_USEANIMATIONLINKAGE
	}

	Super::Serialize(Ar);
}

/** 
* Gets the details in a human readable string representation
* @param outDetails The resulting details string
*/
void UFaceFXActor::GetDetails(FString& OutDetails) const
{
	const bool bIsValid = IsValid();

	OutDetails = LOCTEXT("DetailsActorHeader", "FaceFX Actor").ToString() + TEXT("\n\n");
	OutDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + AssetName + TEXT("\n");
	
	if(bIsValid)
	{
		const int32 EstSize = GetData().ActorRawData.Max() + GetData().BonesRawData.Max() + GetData().Ids.GetTypeSize() * GetData().Ids.Max();
		OutDetails += LOCTEXT("DetailsSize", "Estimated Size: ").ToString() + FString::FromInt(EstSize) + TEXT(" Bytes\n");
	}

#if FACEFX_USEANIMATIONLINKAGE
	//Animation references
	TArray<FString> AnimRefs;
	for(UFaceFXAnim* AnimRef : Animations)
	{
		if(AnimRef)
	{
			AnimRefs.Add(AnimRef->GetId().GetIdString());
		}
	}

	OutDetails += TEXT("\n");

	if(AnimRefs.Num() > 0)
	{
		AnimRefs.Sort();

		OutDetails += LOCTEXT("DetailsActorAnimRefs", "Animation References").ToString() + FString::Printf(TEXT(" (%i)\n"), AnimRefs.Num());
		OutDetails += TEXT("-----------------------------\n");
		for(auto& AnimRef : AnimRefs)
		{
			OutDetails += AnimRef + TEXT("\n");
		}

		OutDetails += TEXT("\n");
	}

#endif //FACEFX_USEANIMATIONLINKAGE

	//Ids
	if(bIsValid)
	{
		auto& Ids = PlatformData[0].Ids;

		TArray<FString> SortedBones;
		SortedBones.Reserve(Ids.Num());

		OutDetails += LOCTEXT("DetailsSetIds", "IDs").ToString() + FString::Printf(TEXT(" (%i)\n"), Ids.Num());
		OutDetails += TEXT("-----------------------------\n");

		for(auto& Id : Ids)
		{
			SortedBones.Add(Id.Name.GetPlainNameString());
		}

		SortedBones.Sort();
		for(auto& Id : SortedBones)
		{
			OutDetails += Id + TEXT("\n");
		}
	}
	else
	{
		OutDetails += LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

#endif //WITH_EDITORONLY_DATA

#if WITH_EDITORONLY_DATA && FACEFX_USEANIMATIONLINKAGE

int32 UFaceFXActor::GetAnimationCount() const
{
	int32 Result = 0;
	for(auto AnimSet : Animations)
	{
		if(AnimSet)
		{
			Result += AnimSet->GetAnimationCount();
		}
	}
	return Result;
}

#endif //WITH_EDITORONLY_DATA && FACEFX_USEANIMATIONLINKAGE

#if FACEFX_USEANIMATIONLINKAGE

const UFaceFXAnim* UFaceFXActor::GetAnimation(const FName& AnimGroup, const FName& AnimName) const
{
	const bool IsNoneGroup = AnimGroup.IsNone();

	for(auto Animation : Animations)
	{
		if(Animation && (IsNoneGroup || Animation->GetGroup() == AnimGroup))
		{
			if(Animation->GetName() == AnimName)
			{
				return Animation;
			}
		}
	}
	return nullptr;
}

void UFaceFXActor::GetAnimationGroups(TArray<FName>& OutGroups) const
{
	for(auto AnimSet : Animations)
	{
		if(AnimSet)
		{
			OutGroups.AddUnique(AnimSet->GetGroup());
		}
	}
}

#endif //FACEFX_USEANIMATIONLINKAGE

#undef LOCTEXT_NAMESPACE
