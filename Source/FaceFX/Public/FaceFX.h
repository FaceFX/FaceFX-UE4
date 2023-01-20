/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2023 OC3 Entertainment, Inc. All rights reserved.
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

#pragma once

#include "FaceFXCharacter.h"
#include "FaceFXActor.h"
#include "FaceFXAnim.h"
#include "FaceFXConfig.h"

FACEFX_API DECLARE_LOG_CATEGORY_EXTERN(LogFaceFX, Display, All);
DECLARE_STATS_GROUP(TEXT("FaceFX"),STATGROUP_FACEFX, STATCAT_Advance);

class UFaceFXAnim;
struct FFaceFXAnimData;

struct FACEFX_API FaceFX
{
	/**
	* Gets the FaceFX version
	* @returns The FaceFX version string
	*/
	static FString GetVersion();

	/**
	* Gets the global streamer for FaceFX assets
	* @returns The streamer
	*/
	static struct FStreamableManager& GetStreamer();

	/**
    * Gets the string representation of an FxResult value.
	* @param Result The FxResult value of a FaceFX Runtime function.
	* @returns A string representation of Result.
	*/
	static FString GetFaceFXResultString(FxResult Result);

	/**
	* Loads a set of animation data
	* @param AnimData The data to load the animation with
	* @returns The FaceFX handle if succeeded, else nullptr
	*/
	static FxAnimation LoadAnimation(const FFaceFXAnimData& AnimData);
	
	/**
	* Gets the start and end time of a given animation
	* @param pAnimation The animation to fetch the bounds for
	* @param Start The start time if call succeeded
	* @param End The end time if call succeeded
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationBounds(const UFaceFXAnim* pAnimation, float& Start, float& End);

private:

	FaceFX() {}
};
