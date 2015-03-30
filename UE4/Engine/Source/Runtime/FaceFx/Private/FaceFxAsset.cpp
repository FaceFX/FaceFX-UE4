#include "FaceFx.h"
#include "TargetPlatform.h"

UFaceFxAsset::UFaceFxAsset(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITOR

/** 
* Initializes the asset from a .facefx asset file
* @param file The path to the .facefx asset file
* @param errorMessage The optional error message in case it failed
* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
* @returns True if succeeded, else false
*/
bool UFaceFxAsset::InitializeFromFaceFx(const FString& file, FString* outErrorMessage, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback)
{
	if(!FPaths::FileExists(file))
	{
		//file not exist
		UE_LOG(LogFaceFx, Error, TEXT("UFaceFxAsset::InitializeFromFaceFx. FaceFX asset file does not exist: %s"), *file);
		return false;
	}

	m_assetFolder = FPaths::GetPath(file);
	m_assetName = FPaths::GetBaseFilename(file);

	return FFaceFxEditorTools::ImportFaceFXAsset(this, file, outErrorMessage, beforeDeletionCallback) && IsValid();
}

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