/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2017 OC3 Entertainment, Inc.
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

#include "Matinee/InterpTrackInst.h"
#include "FaceFXMatineeControlInst.generated.h"

UCLASS(MinimalAPI)
class UFaceFXMatineeControlInst : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** The position at which the last time the update was performed */
	UPROPERTY(Transient)
	float LastUpdatePosition;

	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) override;
	virtual void RestoreActorState(UInterpTrack* Track) override;
	// End UInterpTrackInst Instance

	/**
	* Gets the current track index for the given skel mesh component
	* @param SkelMeshComp The skel mesh component to fetch the track index for
	* @returns The track index or INDEX_NONE if unset
	*/
	inline int32 GetCurrentTrackIndex(const class USkeletalMeshComponent* SkelMeshComp) const
	{
		if(const int32* Entry = CurrentTrackKeyIndices.Find(SkelMeshComp))
		{
			return *Entry;
		}
		return INDEX_NONE;
	}

	/**
	* Sets the current track index for the given skel mesh component
	* @param SkelMeshComp The skel mesh component to set the track index for
	* @param Index The new index to set
	*/
	inline void SetCurrentTrackIndex(const class USkeletalMeshComponent* SkelMeshComp, int32 Index)
	{
		CurrentTrackKeyIndices.FindOrAdd(SkelMeshComp) = Index;
	}

private:

	/** The currently executed track key on a per skelmesh component level*/
	TMap<const class USkeletalMeshComponent*, int32> CurrentTrackKeyIndices;
};

