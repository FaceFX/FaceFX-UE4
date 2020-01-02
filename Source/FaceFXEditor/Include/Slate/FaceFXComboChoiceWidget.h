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

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

/** A widget that shows a drop down selection box of different strings the user can select from */
class FFaceFXComboChoiceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(FFaceFXComboChoiceWidget)	{}
	SLATE_ATTRIBUTE(FText, Message)
	SLATE_ATTRIBUTE(TArray<FString>, Options)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	/**
	* Gets the dialog response at the time of closure
	* @returns The response type
	*/
	inline EAppReturnType::Type GetResponse() const
	{
		return Response;
	}

	virtual	FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyboardEvent) override;

	/** Override the base method to allow for keyboard focus */
	virtual bool SupportsKeyboardFocus() const  override
	{
		return true;
	}

	/**
	* Gets the currently selected option
	* @returns The selected option. Nullptr if none selected
	*/
	inline FString* GetSelectedOption() const
	{
		return ComboBox->GetSelectedItem().Get();
	}

	/**
	* Opens this widget as a modal dialog
	* @param InTitle The title for the modal window
	* @param ShowAsModal Indicator if this widget shall be displayed in a modal window
	* @returns The return type
	*/
	EAppReturnType::Type OpenDialog(const FText& InTitle, bool ShowAsModal = true);

	/**
	* Creates a result dialog
	* @param InTitle The window title
	* @param InMessage The message
	* @param Options The choices to display
	* @param OutResult The created widget
	* @param ShowAsModal Indicator if this widget shall be displayed in a modal window
	* @returns The return type from the dialog
	*/
	static EAppReturnType::Type Create(const FText& InTitle, const FText& InMessage, const TArray<FString>& Options, TSharedPtr<FFaceFXComboChoiceWidget>& OutResult, bool ShowAsModal = false);

private:

	FReply HandleButtonClicked(EAppReturnType::Type Response_);

	/**
	* Generates the widget for a given combo item option
	* @param Value The value to create the widget for
	* @returns The new widget
	*/
	TSharedRef<SWidget> MakeComboItemWidget(TSharedPtr<FString> Value);

	void OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** The parent window */
	TSharedPtr<SWindow> Parent;

	/** The combo box containing all possible options */
	TSharedPtr< SComboBox< TSharedPtr<FString> > > ComboBox;

	/** The options list */
	TArray< TSharedPtr<FString> > Options;

	/** The last response type */
	EAppReturnType::Type Response;
};
