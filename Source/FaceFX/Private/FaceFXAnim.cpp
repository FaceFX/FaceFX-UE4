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

#include "FaceFXAnim.h"
#include "FaceFX.h"
#include "Sound/SoundWave.h"

#if WITH_EDITORONLY_DATA
#include "FaceFXBlueprintLibrary.h"
#endif //WITH_EDITORONLY_DATA

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXAnim::UFaceFXAnim(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA

bool UFaceFXAnim::GetAbsoluteAudioPath(FString& OutResult) const
{
	if (!AudioPath.IsEmpty())
	{
		OutResult = FPaths::IsRelative(AudioPath) ? GetAssetFolder() / AudioPath : AudioPath;
		return true;
	}
	return false;
}

#endif //WITH_EDITORONLY_DATA

void UFaceFXAnim::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	if(CumulativeResourceSize.GetResourceSizeMode() == EResourceSizeMode::Exclusive)
	{
		//only count cooked data without any references
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(sizeof(FFaceFXAnimId));

		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(AnimData.RawData.Num() * AnimData.RawData.GetTypeSize());
	}
	else
	{
		if(USoundWave* AudioPtr = Audio.Get())
		{
			AudioPtr->GetResourceSizeEx(CumulativeResourceSize);
		}
	}
}

#if WITH_EDITORONLY_DATA

void UFaceFXAnim::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && PlatformData_DEPRECATED.Num() > 0)
	{
		checkf(!AnimData.IsValid(), TEXT("Asset in invalid state during load."));

		AnimData = PlatformData_DEPRECATED[0];

		PlatformData_DEPRECATED.Empty();

		UE_LOG(LogFaceFX, Warning, TEXT("Upgraded FaceFXAnim %s:%s/%s. Please re-save."), *AssetName, *(GetGroup().ToString()), *(GetName().ToString()));

		// The editor does not allow you to mark a package as dirty during load, but in this case we need to
		// bypass that enforcement and do it anyway. This is important because the above "upgrade" could
		// potentially reclaim a lot of memory and if we didn't mark the package as dirty the user would have to
		// a) see our warning from above (unlikely) and b) select each asset individually in the content browser
		// and force save.
		UPackage* Package = GetOutermost();

		const bool bIsDirty = Package->IsDirty();

		if(!bIsDirty)
		{
			Package->SetDirtyFlag(true);
		}

		Package->PackageMarkedDirtyEvent.Broadcast(Package, bIsDirty);
	}
}

void UFaceFXAnim::GetDetails(FString& OutDetails) const
{
	OutDetails = LOCTEXT("DetailsAnimSetHeader", "FaceFX Animation").ToString() + TEXT("\n\n");
	OutDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + AssetName + TEXT("\n");
	OutDetails += LOCTEXT("DetailsAnimGroup", "Group: ").ToString() + Id.Group.GetPlainNameString() + TEXT("\n");
	OutDetails += LOCTEXT("DetailsAnimId", "Animation: ").ToString() + Id.Name.GetPlainNameString() + TEXT("\n");

	float Start, End, Duration;
	if (UFaceFXBlueprintLibrary::GetAnimationBounds(this, Start, End, Duration))
	{
		OutDetails += LOCTEXT("DetailsAnimTimeStart", "StartTime: ").ToString() + FString::Printf(TEXT("%0.5fs\n"), Start);
		OutDetails += LOCTEXT("DetailsAnimTimeEnd", "EndTime: ").ToString() + FString::Printf(TEXT("%0.5fs\n"), End);
		OutDetails += LOCTEXT("DetailsAnimTimeDuration", "Duration: ").ToString() + FString::Printf(TEXT("%0.5fs\n"), Duration);
	}

	if(!IsValid())
	{
		OutDetails += TEXT("\n") + LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

#endif //WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE
