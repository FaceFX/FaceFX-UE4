#include "FaceFXEditor.h"
#include "FaceFXDialogs.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "FaceFX"

EAppReturnType::Type FComboboxChoiceWidget::OpenModalDialog(const FText& InTitle)
{
	auto ModalWindow = SNew(SWindow)
		.Title(InTitle)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	Parent = ModalWindow;
	ModalWindow->SetContent(AsShared());

	GEditor->EditorAddModalWindow(ModalWindow);

	return GetResponse();
}

void FComboboxChoiceWidget::Construct(const FArguments& Args)
{
	Response = EAppReturnType::Cancel;

	//copy options
	for(const FString& Option : Args._Options.Get())
	{
		Options.Add(MakeShareable(new FString(Option)));
	}

	FSlateFontInfo MessageFont( FEditorStyle::GetFontStyle(TEXT("StandardDialog.LargeFont")));

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
						.Text(Args._Message)
						.Font(MessageFont)
					]
				]
				+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f)
					[
						SAssignNew(ComboBox, SComboBox< TSharedPtr<FString> >)
						.OptionsSource(&Options)
						.OnSelectionChanged( this, &FComboboxChoiceWidget::OnSelectionChanged )
						.OnGenerateWidget(this, &FComboboxChoiceWidget::MakeComboItemWidget)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ComboBoxSelctionEmpty", "<Please Select>"))
						]
					]
			]
		];
}

void FComboboxChoiceWidget::OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if(NewSelection.IsValid())
	{
		Response = EAppReturnType::Ok;
	}
	if(Parent.IsValid())
	{
		Parent->RequestDestroyWindow();
	}
}

TSharedRef<SWidget> FComboboxChoiceWidget::MakeComboItemWidget(TSharedPtr<FString> Value)
{
	FString* ValuePtr = Value.Get();
	FString ValueS = ValuePtr ? *ValuePtr : TEXT("None");

	return SNew(STextBlock)
		.Text(ValueS)
		.ToolTipText(ValueS);
}

FReply FComboboxChoiceWidget::HandleButtonClicked(EAppReturnType::Type Response)
{
	Response = Response;
	return FReply::Handled();
}

FReply FComboboxChoiceWidget::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyboardEvent)
{
	if(KeyboardEvent.GetKey() == EKeys::Escape)
	{
		return HandleButtonClicked(EAppReturnType::Cancel);
	}
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE