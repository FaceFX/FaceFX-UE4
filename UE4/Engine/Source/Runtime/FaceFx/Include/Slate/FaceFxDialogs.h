#if WITH_EDITOR

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

	void Construct(const FArguments& args);

	/**
	* Gets the dialog response at the time of closure
	* @returns The response type
	*/
	inline EAppReturnType::Type GetResponse() const
	{
		return m_response;
	}

	virtual	FReply OnKeyDown(const FGeometry& geometry, const FKeyEvent& keyboardEvent) override;

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
		return m_comboBox->GetSelectedItem().Get();
	}

	/**
	* Opens this widget as a modal dialog
	* @param InTitle The title for the modal window
	* @returns The return type
	*/
	EAppReturnType::Type OpenModalDialog(const FText& title);

private:

	FReply HandleButtonClicked(EAppReturnType::Type response);

	/**
	* Generates the widget for a given combo item option
	* @param value The value to create the widget for
	* @returns The new widget
	*/
	TSharedRef<SWidget> MakeComboItemWidget(TSharedPtr<FString> value);

	void OnSelectionChanged(TSharedPtr<FString> newSelection, ESelectInfo::Type selectInfo);

	/** The parent window */
	TSharedPtr<SWindow> m_parent;

	/** The combo box containing all possible options */
	TSharedPtr< SComboBox< TSharedPtr<FString> > > m_comboBox;;

	/** The options list */
	TArray< TSharedPtr<FString> > m_options;

	/** The last response type */
	EAppReturnType::Type m_response;
};

#endif