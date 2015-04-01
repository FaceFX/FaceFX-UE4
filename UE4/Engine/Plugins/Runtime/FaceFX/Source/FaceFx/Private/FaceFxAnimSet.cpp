#include "FaceFx.h"

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

#endif //WITH_EDITOR

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

#undef LOCTEXT_NAMESPACE