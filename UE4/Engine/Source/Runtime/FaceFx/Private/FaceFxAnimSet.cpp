#include "FaceFx.h"
#include "Include/Slate/FaceFxDialogs.h"

#define LOCTEXT_NAMESPACE "FaceFx"

UFaceFxAnimSet::UFaceFxAnimSet(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITOR

void UFaceFxAnimSet::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving() && Ar.IsCooking())
	{
		ClearPlatformData(Ar, m_platformData);
	}

	Super::Serialize(Ar);
}

/** 
* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
* @param folder The path to the platform folder to load the compiled assets from
* @param outErrorMessage The optional error message in case it failed
* @returns True if succeeded, else false
*/
bool UFaceFxAnimSet::LoadFromFaceFxCompilationFolder(const FString& folder, FString* outErrorMessage)
{
	if(!IFileManager::Get().DirectoryExists(*folder))
	{
		//file not exist
		if(outErrorMessage)
		{
			*outErrorMessage = FString::Printf(*LOCTEXT("MissingCompilationFolder", "FaceFX compilation folder does not exist: %s").ToString(), *folder);
		}
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	if(m_group.IsNone())
	{
		//no group defined yet -> ask user for group
		
		//fetch all animation groups
		TArray<FString> groups;
		FFaceFxEditorTools::GetAnimationGroupsInFolder(folder, EFaceFxTargetPlatform::PC, groups);

		if(groups.Num() > 0)
		{
			auto selectionWidget = SNew(FComboboxChoiceWidget)
				.Options(groups)
				.Message(LOCTEXT("SelectAnimGroupComboTitle", "Animation Groups:"));

			const EAppReturnType::Type result = selectionWidget->OpenModalDialog(LOCTEXT("SelectAnimGroupTitle", "Select Animation Group"));

			if(result == EAppReturnType::Ok)
			{
				if(FString* selection = selectionWidget->GetSelectedOption())
				{
					m_group = FName(**selection);
				}
				else
				{
					//no valid response
					if(outErrorMessage)
					{
						*outErrorMessage = LOCTEXT("NoAnimGroupSelected", "No Animation Groups Selected").ToString();
					}
					return false;
				}
			}
			else
			{
				//canceled
				if(outErrorMessage)
				{
					*outErrorMessage = LOCTEXT("ImportCancelled", "Import Cancelled").ToString();
				}
				return false;
			}
		}
		else
		{
			//no groups
			if(outErrorMessage)
			{
				*outErrorMessage = LOCTEXT("NoAnimGroups", "No Animation Groups Found").ToString();
			}
			UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not contain any animation group: %s"), *folder);
			return false;
		}
	}

	if(m_group.IsNone())
	{
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAnimSet::LoadFromFaceFxCompilationFolder. FaceFX compilation folder does not exist: %s"), *folder);
		return false;
	}

	//reset platform data
	m_platformData.Reset(EFaceFxTargetPlatform::MAX);
	m_platformData.AddZeroed(EFaceFxTargetPlatform::MAX);

	bool result = true;

	for(int8 i=0; i<EFaceFxTargetPlatform::MAX; ++i)
	{
		const EFaceFxTargetPlatform::Type target = static_cast<EFaceFxTargetPlatform::Type>(i);
		if(!FFaceFxEditorTools::LoadCompiledData(FFaceFxEditorTools::GetCompilationFolder(folder, target) / m_group.GetPlainNameString(), m_platformData[target]))
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
* Gets the details in a human readable string representation
* @param outDetails The resulting details string
*/
void UFaceFxAnimSet::GetDetails(FString& outDetails) const
{
	outDetails = LOCTEXT("DetailsAnimSetHeader", "FaceFX Animation Set").ToString() + TEXT("\n\n");
	outDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + m_assetName + TEXT("\n");
	outDetails += LOCTEXT("DetailsAnimGroup", "Group: ").ToString() + m_group.GetPlainNameString() + TEXT("\n\n");
	
	if(IsValid())
	{
		auto& animations = GetData().m_animations;

		outDetails += LOCTEXT("DetailsAnimAnimationsHeader", "Animations").ToString() + TEXT(" (") + FString::FromInt(animations.Num()) + TEXT(")\n");
		outDetails += TEXT("-----------------------------\n");

		for(auto& anim : animations)
		{
			outDetails += anim.m_name.GetPlainNameString() + TEXT("\n");
		}
	}
	else
	{
		outDetails += LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE