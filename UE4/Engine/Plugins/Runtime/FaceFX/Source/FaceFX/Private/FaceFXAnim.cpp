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

#include "Sound/SoundWave.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXAnim::UFaceFXAnim(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

SIZE_T UFaceFXAnim::GetResourceSize(EResourceSizeMode::Type Mode)
{
	SIZE_T ResSize = Super::GetResourceSize(Mode);

	if(Mode == EResourceSizeMode::Exclusive)
	{
		//only count cooked data without any references
		ResSize += sizeof(FFaceFXAnimId);

		if(PlatformData.Num() > 0)
		{
			//take the first entry as an approximation
			const FFaceFXAnimData& Data = PlatformData[0];
			ResSize += Data.RawData.Num() * Data.RawData.GetTypeSize();
		}
	}
	else
	{
		if(USoundWave* AudioPtr = Audio.Get())
		{
			ResSize += AudioPtr->GetResourceSize(Mode);
		}
	}

	return ResSize;
}

#if WITH_EDITORONLY_DATA

void UFaceFXAnim::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving() && Ar.IsCooking())
	{
		ClearPlatformData(Ar, PlatformData);
	}

	Super::Serialize(Ar);
}

void UFaceFXAnim::GetDetails(FString& OutDetails) const
{
	OutDetails = LOCTEXT("DetailsAnimSetHeader", "FaceFX Animation").ToString() + TEXT("\n\n");
	OutDetails += LOCTEXT("DetailsSource", "Source: ").ToString() + AssetName + TEXT("\n");
	OutDetails += LOCTEXT("DetailsAnimGroup", "Group: ").ToString() + Id.Group.GetPlainNameString() + TEXT("\n");
	OutDetails += LOCTEXT("DetailsAnimId", "Animation: ").ToString() + Id.Name.GetPlainNameString() + TEXT("\n");
	
	if(!IsValid())
	{
		OutDetails += TEXT("\n") + LOCTEXT("DetailsNotLoaded", "No FaceFX data").ToString();
	}
}

#endif //WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE