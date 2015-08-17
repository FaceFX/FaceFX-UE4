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

#pragma once

class UInterpTrack;
class UInterpGroup;

#include "FaceFXConfig.h"
#if FACEFX_USEANIMATIONLINKAGE
#include "FaceFXData.h"
#endif //FACEFX_USEANIMATIONLINKAGE

#include "InterpTrackHelper.h"
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

	/** Resets the cached values */
	inline void ResetCachedValues() const
	{
		KeyframeAddFaceFXAnim = nullptr;
#if FACEFX_USEANIMATIONLINKAGE
		KeyframeAddFaceFXAnimId.Reset();
#endif //FACEFX_USEANIMATIONLINKAGE
		KeyframeAddSkelMeshComponentId.Reset();
		bKeyframeSettingsLoop = false;
	}

	/** 
	* Event callback for when the FaceFX anim asset was selected 
	* @param AssetData The asset selection data
	* @param Matinee The matinee instance
	* @param Track THe track for which the asset was selected
	*/
	void OnAnimAssetSelected(const class FAssetData& AssetData, class IMatineeBase* Matinee, UInterpTrack* Track);

	/** 
	* Event callback for when the keyframe loop setting checkbox got (un)checked
	* @param NewState The new state of the checkbox
	*/
	void OnKeyframeLoopCheckboxChange(ECheckBoxState NewState);

	/** 
	* Event callback for when the user selected an skelmesh component from the combo box
	* @param NewSelection The new selection
	* @param SelectInfo The selection info
	* @param Matinee The matinee instance
	* @param Track THe track for which the asset was selected
	*/
	void OnSkelMeshComboBoxSelected(TSharedPtr<struct FFaceFXSkelMeshSelection> NewSelection, enum ESelectInfo::Type SelectInfo, class IMatineeBase* Matinee, UInterpTrack* Track);

	/** Creates the widget for the skelmesh selection combo box entry */
	TSharedRef<SWidget> MakeWidgetFromSkelMeshSelection(TSharedPtr<struct FFaceFXSkelMeshSelection> InItem);

#if FACEFX_USEANIMATIONLINKAGE
	/** 
	* Event callback when the FaceFX animation group was selected
	* @param Text The committed text
	* @param Type The commit type
	* @param Matinee The matinee instance
	* @param Track THe track for which the asset was selected
	*/
	void OnAnimGroupCommitted(const FText& Text, ETextCommit::Type Type, class IMatineeBase* Matinee, UInterpTrack* Track);

	/** 
	* Event callback when the FaceFX animation id was selected
	* @param Text The committed text
	* @param Type The commit type
	* @param Matinee The matinee instance
	* @param Track THe track for which the asset was selected
	*/
	void OnAnimIdCommitted(const FText& Text, ETextCommit::Type Type, class IMatineeBase* Matinee, UInterpTrack* Track);

	/** Creates the widget for the animation id combo box entry */
	TSharedRef<SWidget> MakeWidgetFromAnimId(TSharedPtr<FFaceFXAnimId> InItem);

	/** 
	* Event callback for when the user selected an animation id from the combo box
	* @param NewSelection The new selection
	* @param SelectInfo The selection info
	* @param Matinee The matinee instance
	* @param Track THe track for which the asset was selected
	*/
	void OnAnimIdComboBoxSelected(TSharedPtr<FFaceFXAnimId> NewSelection, enum ESelectInfo::Type SelectInfo, class IMatineeBase* Matinee, UInterpTrack* Track);

	/** The FaceFX animation id that is currently being added */
	mutable FFaceFXAnimId KeyframeAddFaceFXAnimId;

	/** The animation id combo box entries */
	mutable TArray<TSharedPtr<FFaceFXAnimId>> KeyframeAddFaceDXExistingAnimIds;

#endif //FACEFX_USEANIMATIONLINKAGE

	/** */
	mutable TSharedPtr<class STextBlock> SkelMeshComponentSelection;

	/** The combo box entries for the skel mesh combo box */
	mutable TArray<TSharedPtr<struct FFaceFXSkelMeshSelection>> SkelMeshSelectionComboBoxEntries;

	/** The FaceFX animation that is currently being added */
	UPROPERTY()
	mutable class UFaceFXAnim* KeyframeAddFaceFXAnim;

	/** The currently selected skel mesh component */
	mutable FFaceFXSkelMeshComponentId KeyframeAddSkelMeshComponentId;

	/** The last popup window that was opened */
	mutable TWeakPtr<class SWindow> EntryPopupWindow;

	/** Indicator if the keyframe that is about to get created shall loop the animation */
	mutable uint8 bKeyframeSettingsLoop : 1;
};