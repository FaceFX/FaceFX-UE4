#include "FaceFX.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXAnimSet::UFaceFXAnimSet(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITOR

void UFaceFXAnimSet::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving() && Ar.IsCooking())
	{
		ClearPlatformData(Ar, PlatformData);
	}

	Super::Serialize(Ar);
}

#endif //WITH_EDITOR

void UFaceFXAnimSet::GetDetails(FString& OutDetails) const
{
	OutDetails = LOCTEXT("DetailsAnimSetHeader", "FaceFX Animation Set").ToString() + TEXT("\n\n");
	OutDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + AssetName + TEXT("\n");
	OutDetails += LOCTEXT("DetailsAnimGroup", "Group: ").ToString() + Group.GetPlainNameString() + TEXT("\n\n");
	
	if(IsValid())
	{
		auto& Animations = GetData().Animations;

		OutDetails += LOCTEXT("DetailsAnimAnimationsHeader", "Animations").ToString() + TEXT(" (") + FString::FromInt(Animations.Num()) + TEXT(")\n");
		OutDetails += TEXT("-----------------------------\n");

		for(auto& Anim : Animations)
		{
			OutDetails += Anim.Name.GetPlainNameString() + TEXT("\n");
		}
	}
	else
	{
		OutDetails += LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

#undef LOCTEXT_NAMESPACE