#include "FaceFx.h"
#include "FaceFxData.h"
#include "TargetPlatform.h"

UFaceFxAsset::UFaceFxAsset(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITOR

/** 
* Clear platform specific data based on the target Archive platform
* @param Ar The archive to use
* @param platformData The data to clear
*/
template <typename T> void UFaceFxAsset::ClearPlatformData(const FArchive& Ar, T& platformData)
{
	//check if we actually have platform specific data to store
	if(platformData.Num() >= EFaceFxTargetPlatform::MAX)
	{
		const FString platform = Ar.CookingTarget()->PlatformName();

		//Indices: x86, PS4, XBoxOne
		//copy the one we want to the first index
		if(platform.Equals("PS4", ESearchCase::IgnoreCase))
		{
			platformData.Swap(EFaceFxTargetPlatform::PC, EFaceFxTargetPlatform::PS4);
		}
		else if(platform.Equals("XBoxOne", ESearchCase::IgnoreCase))
		{
			platformData.Swap(EFaceFxTargetPlatform::PC, EFaceFxTargetPlatform::XBoxOne);
		}
		//else no change required as we want the first one only

		//remove anything but the first position
		platformData.RemoveAt(1, platformData.Num() - 1);
	}
}

#endif