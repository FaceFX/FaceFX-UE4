#if WITH_EDITOR

#include "FaceFx.h"
#include "FaceFxDialogs.h"
#include "EditorStyle.h"

/**
* Opens this widget as a modal dialog
* @param InTitle The title for the modal window
* @returns The return type
*/
EAppReturnType::Type FComboboxChoiceWidget::OpenModalDialog(const FText& title)
{
	auto modalWindow = SNew(SWindow)
		.Title(title)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	m_parent = modalWindow;
	modalWindow->SetContent(AsShared());

	GEditor->EditorAddModalWindow(modalWindow);

	return GetResponse();
}

void FComboboxChoiceWidget::Construct(const FArguments& args)
{
	m_response = EAppReturnType::Cancel;

	//copy options
	for(const FString& option : args._Options.Get())
	{
		m_options.Add(MakeShareable(new FString(option)));
	}

	FSlateFontInfo MessageFont( FEditorStyle::GetFontStyle("StandardDialog.LargeFont"));

	ChildSlot
		[	
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.FillHeight(1.0f)
				.MaxHeight(550)
				.Padding(12.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(STextBlock)
						.Text(args._Message)
						.Font(MessageFont)
					]
				]
				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f)
					[
						SAssignNew(m_comboBox, SComboBox< TSharedPtr<FString> >)
						.OptionsSource(&m_options)
						.OnSelectionChanged( this, &FComboboxChoiceWidget::OnSelectionChanged )
						.OnGenerateWidget(this, &FComboboxChoiceWidget::MakeComboItemWidget)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("FaceFX", "ComboBoxSelctionEmpty", "<Please Select>"))
						]
					]
			]
		];
}

void FComboboxChoiceWidget::OnSelectionChanged(TSharedPtr<FString> newSelection, ESelectInfo::Type selectInfo)
{
	if(newSelection.IsValid())
	{
		m_response = EAppReturnType::Ok;
	}
	if(m_parent.IsValid())
	{
		m_parent->RequestDestroyWindow();
	}
}

/**
* Generates the widget for a given combo item option
* @param value The value to create the widget for
* @returns The new widget
*/
TSharedRef<SWidget> FComboboxChoiceWidget::MakeComboItemWidget(TSharedPtr<FString> value)
{
	FString* valuePtr = value.Get();
	FString valueS = valuePtr ? *valuePtr : "None";

	return SNew(STextBlock)
		.Text(valueS)
		.ToolTipText(valueS);
}

FReply FComboboxChoiceWidget::HandleButtonClicked(EAppReturnType::Type response)
{
	m_response = response;
	return FReply::Handled();
}

FReply FComboboxChoiceWidget::OnKeyDown(const FGeometry& geometry, const FKeyEvent& keyboardEvent)
{
	if(keyboardEvent.GetKey() == EKeys::Escape)
	{
		return HandleButtonClicked(EAppReturnType::Cancel);
	}
	return FReply::Unhandled();
}

#endif