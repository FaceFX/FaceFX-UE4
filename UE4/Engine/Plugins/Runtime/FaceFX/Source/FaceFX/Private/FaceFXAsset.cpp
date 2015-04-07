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
#include "FaceFXData.h"
#include "TargetPlatform.h"

UFaceFXAsset::UFaceFXAsset(const class FObjectInitializer& PCIP) : Super(PCIP)
{
}

#if WITH_EDITOR

/** 
* Clear platform specific data based on the target Archive platform
* @param Ar The archive to use
* @param platformData The data to clear
*/
template <typename T> void UFaceFXAsset::ClearPlatformData(const FArchive& Ar, T& platformData)
{
	//check if we actually have platform specific data to store
	if(platformData.Num() >= EFaceFXTargetPlatform::MAX)
	{
		const FString platform = Ar.CookingTarget()->PlatformName();

		//Indices: x86, PS4, XBoxOne
		//copy the one we want to the first index
		if(platform.Equals("PS4", ESearchCase::IgnoreCase))
		{
			platformData.Swap(EFaceFXTargetPlatform::PC, EFaceFXTargetPlatform::PS4);
		}
		else if(platform.Equals("XBoxOne", ESearchCase::IgnoreCase))
		{
			platformData.Swap(EFaceFXTargetPlatform::PC, EFaceFXTargetPlatform::XBoxOne);
		}
		//else no change required as we want the first one only

		//remove anything but the first position
		platformData.RemoveAt(1, platformData.Num() - 1);
	}
}

#endif
