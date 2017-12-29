/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2018 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXAsset.h"
#include "FaceFX.h"

#if WITH_EDITORONLY_DATA
#include "FaceFXData.h"
#include "Paths.h"
#include "ITargetPlatform.h"
#endif

UFaceFXAsset::UFaceFXAsset(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA

FString UFaceFXAsset::GetAssetPathAbsolute() const
{
	return FPaths::ConvertRelativePathToFull(GetAssetPath());
}

FString UFaceFXAsset::GetAssetFolderAbsolute() const
{
	return FPaths::ConvertRelativePathToFull(AssetFolder);
}

template <typename T> bool UFaceFXAsset::ClearPlatformData(const FArchive& Ar, T& PlatformData)
{
	//check if we actually have platform specific data to store
	//The list of available platforms are: WindowsNoEditor, WindowsServer, LinuxServer, PS4, XboxOne, Switch, IOS, and Android.
	const FString Platform = Ar.CookingTarget()->PlatformName();

	EFaceFXTargetPlatform::Type TargetPlatform = EFaceFXTargetPlatform::PC;

	//Indices: x86, PS4, XBoxOne
	//copy the one we want to the first index
#if FACEFX_SUPPORT_PS4
	if(Platform.Equals(TEXT("PS4"), ESearchCase::IgnoreCase))
	{
		TargetPlatform = EFaceFXTargetPlatform::PS4;
	}
	else
#endif //FACEFX_SUPPORT_PS4
#if FACEFX_SUPPORT_XBONE
	if(Platform.Equals("XBoxOne", ESearchCase::IgnoreCase))
	{
		TargetPlatform = EFaceFXTargetPlatform::XBoxOne;
	}
	else
#endif //FACEFX_SUPPORT_XBONE
#if FACEFX_SUPPORT_SWITCH
	if(Platform.Equals("Switch", ESearchCase::IgnoreCase))
	{
		TargetPlatform = EFaceFXTargetPlatform::Switch;
	}
	else
#endif //FACEFX_SUPPORT_SWITCH
	if(!Platform.Equals(TEXT("WindowsNoEditor"), ESearchCase::IgnoreCase) && !Platform.Equals(TEXT("WindowsServer"), ESearchCase::IgnoreCase))
	{
		//Unknown target platform
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXAsset::ClearPlatformData. The cooking platform %s is not supported by FaceFX. Asset: %s"), *Platform, *GetNameSafe(this));
		return false;
	}

	//fetch entry for the target platform and move it to the first entry
	const int32 TargetIdx = PlatformData.IndexOfByKey(TargetPlatform);
	if(TargetIdx == INDEX_NONE)
	{
		//target platform data is missing -> ignore and show error
		UE_LOG(LogFaceFX, Error, TEXT("UFaceFXAsset::ClearPlatformData. The FaceFX data for platform %s are missing. To fix this you need to reimport that asset. Asset: %s"), *EFaceFXTargetPlatformHelper::ToString(TargetPlatform), *GetNameSafe(this));
		return false;
	}

	PlatformData.Swap(0, TargetIdx);

	//remove anything but the first position
	PlatformData.RemoveAt(1, PlatformData.Num() - 1);

	return true;
}

// Explicitly instantiate the implementations we need since the definition is in a .cpp file here.
template bool UFaceFXAsset::ClearPlatformData<TArray<FFaceFXActorData>>(const class FArchive& Ar, TArray<FFaceFXActorData>&);
template bool UFaceFXAsset::ClearPlatformData<TArray<FFaceFXAnimData>>(const class FArchive& Ar, TArray<FFaceFXAnimData>&);

#endif //WITH_EDITORONLY_DATA
