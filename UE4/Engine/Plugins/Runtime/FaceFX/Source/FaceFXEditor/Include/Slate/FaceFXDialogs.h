/** Slate based dialogs that are used by the FaceFX integration */

#pragma once

#include "Engine.h"
#include "SlateBasics.h"

/** A widget that shows a drop down selection box of different strings the user can select from */
class FComboboxChoiceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(FComboboxChoiceWidget)	{}
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
	* @returns The return type
	*/
	EAppReturnType::Type OpenModalDialog(const FText& InTitle);

private:

	FReply HandleButtonClicked(EAppReturnType::Type Response);

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