/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2019 OC3 Entertainment, Inc. All rights reserved.
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

class UInterpTrack;
class UInterpGroup;
class IMatineeBase;

#include "InterpTrackHelper.h"
#include "Include/Slate/FaceFXAnimationKeyDetailsDialog.h"
#include "FaceFXMatineeControlHelper.generated.h"

/** Interp helper functionalities for the key frame/track creation handling */
UCLASS()
class UFaceFXMatineeControlHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()

	//UInterpTrackHelper
	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const override;
	virtual	bool PreCreateKeyframe( UInterpTrack *Track, float KeyTime ) const override;
	virtual void PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const override;
	//~UInterpTrackHelper

private:

	/** Callback for when the key properties dialog closed */
	void OnKeyDialogClose(IMatineeBase* Matinee, UInterpTrack* Track) const;

	/** The key creation dialog */
	mutable FFaceFXAnimationKeyDetailsDialog KeyDialog;
};
