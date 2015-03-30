#include "FaceFx.h"

#define LOCTEXT_NAMESPACE "FaceFx"

UFaceFxActor::UFaceFxActor(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

/**
* Gets a specific animation data entry from the set
* @param animGroup the animation group to look for
* @param animName The animation to look for
* @returns The animation data if found, else nullptr
*/
const FFaceFxAnimData* UFaceFxActor::GetAnimation(const FName& animGroup, const FName& animName) const
{
	const bool isNoneGroup = animGroup.IsNone();

	for(auto animSet : m_animationSets)
	{
		if(animSet && (isNoneGroup || *animSet == animGroup))
		{
			auto result = animSet->FindAnimation(animName);
			if(isNoneGroup && !result)
			{
				//animation not found and no group specified -> look further
				continue;
			}
			return result;
		}
	}
	return nullptr;
}

#if WITH_EDITOR

void UFaceFxActor::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		if(Ar.IsCooking())
		{
			ClearPlatformData(Ar, m_platformData);
		}

		//cleanup references to deleted assets
		for(int32 i=m_animationSets.Num()-1; i>=0; --i)
		{
			if(!m_animationSets[i])
			{
				m_animationSets.RemoveAt(i);
			}
		}
	}

	Super::Serialize(Ar);
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool UFaceFxActor::LoadFromFaceFxCompilationFolder(const FString& folder, FString* outErrorMessage)
{
	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//file not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *folder);
		}
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxActor::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	//reset platform data
	m_platformData.Reset(EFaceFxTargetPlatform::MAX);
	m_platformData.AddZeroed(EFaceFxTargetPlatform::MAX);

	bool result = true;

	for(int8 i=0; i<EFaceFxTargetPlatform::MAX; ++i)
	{
		const EFaceFxTargetPlatform::Type target = static_cast<EFaceFxTargetPlatform::Type>(i);
		if(!FFaceFxEditorTools::LoadCompiledData(FFaceFxEditorTools::GetCompilationFolder(folder, target), m_assetName, m_platformData[target]))
		{
			result = false;

			if(outErrorMessage)
			{
				*outErrorMessage = FString::Printf(*LOCTEXT("LoadingCompiledAssetFailed", "Loading compiled data failed. Folder: %s").ToString(), *folder);
			}
}
	}
	return result;
}

/** 
* Gets the number of animations which are encapsulated in this asset
* @return The animation count
*/
int32 UFaceFxActor::GetAnimationCount() const
{
	int32 result = 0;
	for(auto animSet : m_animationSets)
	{
		if(animSet)
		{
			result += animSet->GetAnimationCount();
		}
	}
	return result;
}

/** 
* Gets the details in a human readable string representation
* @param outDetails The resulting details string
*/
void UFaceFxActor::GetDetails(FString& outDetails) const
{
	const bool isValid = IsValid();

	outDetails = LOCTEXT("DetailsSetHeader", "FaceFX Set").ToString() + TEXT("\n\n");
	outDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + m_assetName + TEXT("\n");
	
	if(isValid)
	{
		const int32 estSize = GetData().m_actorRawData.Max() + GetData().m_bonesRawData.Max() + GetData().m_ids.GetTypeSize() * GetData().m_ids.Max();
		outDetails += LOCTEXT("DetailsSize", "Estimated Size: ").ToString() + FString::FromInt(estSize) + TEXT(" Bytes\n");
	}
	outDetails += LOCTEXT("DetailsAnimations", "Animation Count: ").ToString() + FString::FromInt(GetAnimationCount()) + TEXT("\n\n");


	//Animation references
	TArray<FString> animRefs;
	for(UFaceFxAnimSet* animRef : m_animationSets)
	{
		if(animRef)
	{
			animRefs.Add(animRef->GetName());
		}
	}

	if(animRefs.Num() > 0)
	{
		animRefs.Sort();

		outDetails += LOCTEXT("DetailsSetAnimRefs", "Animation References").ToString() + FString::Printf(TEXT(" (%i)\n"), animRefs.Num());
		outDetails += TEXT("-----------------------------\n");
		for(auto& animRef : animRefs)
		{
			outDetails += animRef + TEXT("\n");
		}

		outDetails += TEXT("\n");
	}

	//Ids
	if(isValid)
	{
		auto& ids = m_platformData[0].m_ids;

		TArray<FString> sortedBones;
		sortedBones.Reserve(ids.Num());

		outDetails += LOCTEXT("DetailsSetIds", "IDs").ToString() + FString::Printf(TEXT(" (%i)\n"), ids.Num());
		outDetails += TEXT("-----------------------------\n");

		for(auto& id : ids)
		{
			sortedBones.Add(id.m_name.GetPlainNameString());
		}

		sortedBones.Sort();
		for(auto& id : sortedBones)
		{
			outDetails += id + TEXT("\n");
		}
	}
	else
	{
		outDetails += LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

/**
* Gets all animation groups
* @param outGroups The animation groups
*/
void UFaceFxActor::GetAnimationGroups(TArray<FName>& outGroups) const
{
	for(auto animSet : m_animationSets)
	{
		outGroups.AddUnique(animSet->GetGroup());
	}
}

/**
* Gets the animation set that matches the given group name
* @param group The group to get the animation set from
* @returns The matching animation set or nullptr if not found
*/
UFaceFxAnimSet* UFaceFxActor::GetAnimationSet(const FName& group) const
{
	for(auto animSet : m_animationSets)
	{
		if(animSet && *animSet == group)
		{
			return animSet;
		}
	}
	return nullptr;
}

#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE