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
