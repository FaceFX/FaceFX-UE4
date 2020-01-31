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

#pragma once

class UInterpTrack;
class UInterpGroup;
class UFaceFXComponent;

#include "FaceFXData.h"
#include "FaceFXAnim.h"

#include "Widgets/Input/SComboBox.h"
#include "ContentBrowserDelegates.h"

/** A generic slate dialog showing options on how to define keys for animation systems */
struct FFaceFXAnimationKeyDetailsDialog
{
	FFaceFXAnimationKeyDetailsDialog(bool showLooping = true) : bIsLooping(false), bIsShowLooping(showLooping), bIsCancelled(true) {}

	bool ShowDialog(UFaceFXComponent* FaceFXComponent, const FSimpleDelegate& OnCloseDelegeate = FSimpleDelegate());

	TSharedRef<SWidget> CreateWidget(UFaceFXComponent* FaceFXComponent, const FSimpleDelegate& OnCloseDelegeate = FSimpleDelegate());

	/**
	* Closes the dialog and fires the closed event
	* @param bCanceled The cancellation indicator to set
	* @param bTriggerOnlyOnWindowExist Trigger closed event only when window actually existed
	*/
	void CloseDialog(bool bCanceled = true, bool bTriggerOnlyOnWindowExist = false);

	/** Resets the cached values */
	inline void Reset()
	{
		SelectionData.Reset();
		bIsLooping = false;
	}

	inline const FFaceFXAnimComponentSet& GetSelectedAnimSet() const
	{
		return SelectionData;
	}

	/**
	* Gets the currently selected looping indicator
	* @returns True if looping is enabled, else false
	*/
	inline bool IsLooping() const
	{
		return bIsLooping;
	}

	/**
	* Gets the indicator if the window was closed by canceling
	* @returns True if canceled, else false
	*/
	inline bool IsCancelled() const
	{
		return bIsCancelled;
	}

private:

	/**
	* Event callback for when the FaceFX anim asset was selected
	* @param AssetData The asset selection data
	*/
	void OnAnimAssetSelected(const FAssetData& AssetData);

	/**
	* Event callback for when the keyframe loop setting checkbox got (un)checked
	* @param NewState The new state of the checkbox
	*/
	void OnKeyframeLoopCheckboxChange(ECheckBoxState NewState);

	/**
	* Event callback for when the user selected an skelmesh component from the combo box
	* @param NewSelection The new selection
	* @param SelectInfo The selection info
	*/
	void OnSkelMeshComboBoxSelected(TSharedPtr<struct FFaceFXSkelMeshSelection> NewSelection, enum ESelectInfo::Type SelectInfo);

	/** Creates the widget for the skelmesh selection combo box entry */
	static TSharedRef<SWidget> MakeWidgetFromSkelMeshSelection(TSharedPtr<struct FFaceFXSkelMeshSelection> InItem);

	/** Creates the widget for the animation id combo box entry */
	static TSharedRef<SWidget> MakeWidgetFromAnimId(TSharedPtr<FFaceFXAnimId> InItem);

#if FACEFX_USEANIMATIONLINKAGE
	/**
	* Event callback when the FaceFX animation group was selected
	* @param Text The committed text
	* @param Type The commit type
	*/
	void OnAnimGroupCommitted(const FText& Text, ETextCommit::Type Type);

	/**
	* Event callback when the FaceFX animation id was selected
	* @param Text The committed text
	* @param Type The commit type
	*/
	void OnAnimIdCommitted(const FText& Text, ETextCommit::Type Type);

	/**
	* Event callback for when the user selected an animation id from the combo box
	* @param NewSelection The new selection
	* @param SelectInfo The selection info
	*/
	void OnAnimIdComboBoxSelected(TSharedPtr<FFaceFXAnimId> NewSelection, enum ESelectInfo::Type SelectInfo);

	/** The animation id combo box entries */
	TArray<TSharedPtr<FFaceFXAnimId>> ExistingAnimIds;

#endif //FACEFX_USEANIMATIONLINKAGE

	/** */
	TSharedPtr<class STextBlock> SkelMeshComponentSelection;

	/** The combo box entries for the skel mesh combo box */
	TArray<TSharedPtr<struct FFaceFXSkelMeshSelection>> SkelMeshSelectionComboBoxEntries;

	/** The data that is selected right now */
	FFaceFXAnimComponentSet SelectionData;

	/** The last popup menu that was opened */
	TWeakPtr<IMenu> EntryPopupMenu;

	/** Indicator if the keyframe that is about to get created shall loop the animation */
	uint8 bIsLooping : 1;

	/** Indicator if the looping checkbox shall be shown */
	uint8 bIsShowLooping : 1;

	/** Indicator if the dialog was closed by canceling */
	uint8 bIsCancelled : 1;

	/** Delegate to call when the dialog closes */
	FSimpleDelegate OnDialogCloseDelegate;
};
