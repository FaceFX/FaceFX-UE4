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
#include "FaceFXComboChoiceWidget.h"
#include "FaceFXEditor.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SComboBox.h"

#define LOCTEXT_NAMESPACE "FaceFX"

EAppReturnType::Type FFaceFXComboChoiceWidget::OpenDialog(const FText& InTitle, bool ShowAsModal)
{
	auto Window = SNew(SWindow)
		.Title(InTitle)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	Parent = Window;
	Window->SetContent(AsShared());

	if(ShowAsModal)
	{
		GEditor->EditorAddModalWindow(Window);
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}

	return GetResponse();
}

EAppReturnType::Type FFaceFXComboChoiceWidget::Create(const FText& InTitle, const FText& InMessage, const TArray<FString>& Options, TSharedPtr<FFaceFXComboChoiceWidget>& OutResult, bool ShowAsModal)
{
	auto ResultWidget = SAssignNew(OutResult, FFaceFXComboChoiceWidget).Options(Options).Message(InMessage);
	return ResultWidget->OpenDialog(InTitle, ShowAsModal);
}

void FFaceFXComboChoiceWidget::Construct(const FArguments& Args)
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
						.OnSelectionChanged( this, &FFaceFXComboChoiceWidget::OnSelectionChanged )
						.OnGenerateWidget(this, &FFaceFXComboChoiceWidget::MakeComboItemWidget)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ComboBoxSelctionEmpty", "<Please Select>"))
						]
					]
			]
		];
}

void FFaceFXComboChoiceWidget::OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
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

TSharedRef<SWidget> FFaceFXComboChoiceWidget::MakeComboItemWidget(TSharedPtr<FString> Value)
{
	FString* ValuePtr = Value.Get();
	FString ValueS = ValuePtr ? *ValuePtr : TEXT("None");

	return SNew(STextBlock)
		.Text(FText::FromString(ValueS))
		.ToolTipText(FText::FromString(ValueS));
}

FReply FFaceFXComboChoiceWidget::HandleButtonClicked(EAppReturnType::Type Response_)
{
	Response = Response_;
	return FReply::Handled();
}

FReply FFaceFXComboChoiceWidget::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyboardEvent)
{
	if(KeyboardEvent.GetKey() == EKeys::Escape)
	{
		return HandleButtonClicked(EAppReturnType::Cancel);
	}
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
