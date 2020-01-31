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

#include "FaceFXResultWidget.h"
#include "FaceFXEditor.h"
#include "FaceFXStyle.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Layout/Margin.h"
#include "Misc/MessageDialog.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FaceFX"

static const FName s_ColIdRootImportAsset(TEXT("ImportRootAsset"));
static const FName s_ColIdImportAsset(TEXT("ImportAsset"));
static const FName s_ColIdAsset(TEXT("Asset"));
static const FName s_ColIdSuccess(TEXT("Success"));
static const FName s_ColIdAction(TEXT("Action"));
static const FName s_ColIdMessage(TEXT("Message"));

static const FMargin s_ContentHeaderPadding(8.F, 0.F);

TArray<FFaceFXResultWidget::FResultWidgetInstance> FFaceFXResultWidget::s_OpenInstances;

void FFaceFXAssetRefWidget::Construct(const FArguments& Args)
{
	const FString AssetRefStr = Args._AssetRef.ToString();

	FString AssetRefStrShort = FPackageName::GetShortName(*AssetRefStr);
	AssetRefStrShort.Split(TEXT("."), &AssetRefStrShort, nullptr);

	const float HalfMarginBetweenButtonAndText = 2.5F;

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(FMargin(s_ContentHeaderPadding.Left, s_ContentHeaderPadding.Top, HalfMarginBetweenButtonAndText, s_ContentHeaderPadding.Bottom))
		.MaxWidth(20)
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.OnClicked(this, &FFaceFXAssetRefWidget::OnClicked, Args._AssetRef)
			.ToolTip(SNew(SToolTip).Text(LOCTEXT("ButtonTooltipGotoAsset","Goto Asset")))
			.Visibility(Args._AssetRef.IsValid() ? EVisibility::Visible : EVisibility::Hidden)
			[
				SNew(SImage)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_Browse")))
			]
		]

		+ SHorizontalBox::Slot()
		.Padding(FMargin(HalfMarginBetweenButtonAndText, s_ContentHeaderPadding.Top, s_ContentHeaderPadding.Right, s_ContentHeaderPadding.Bottom))
		[
			SNew(STextBlock)
			.ToolTip(SNew(SToolTip).Text(FText::FromString(AssetRefStr)))
			.Text(FText::FromString(AssetRefStrShort))
		]
	];
}

FReply FFaceFXAssetRefWidget::OnClicked(FSoftObjectPath InAssetRef)
{
	FFaceFXEditorTools::ContentBrowserFocusAsset(InAssetRef);
	return FReply::Handled();
}

FFaceFXResultWidget::~FFaceFXResultWidget()
{
	const int32 IdxOpenWidget = s_OpenInstances.IndexOfByKey(this);
	if(IdxOpenWidget != INDEX_NONE)
	{
		s_OpenInstances.RemoveAtSwap(IdxOpenWidget);
	}

	//run extra cleanup round
	for(int32 i=0; i<s_OpenInstances.Num(); ++i)
	{
		if(!s_OpenInstances[i].IsValid())
		{
			s_OpenInstances.RemoveAt(i);
			--i;
		}
	}
}

void FFaceFXResultWidget::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(SScrollBox)
		+SScrollBox::Slot()
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Menu.Background") )
			//.BorderBackgroundColor(FColor::White)
			.Content()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ListView, SListView<TSharedPtr<ListRowEntry>>)
					.ListItemsSource(&ListViewEntries)
					.OnGenerateRow(this, &FFaceFXResultWidget::GenerateRow)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+SHeaderRow::Column(s_ColIdSuccess).DefaultLabel(LOCTEXT("ResultColSuccess","Result")).FixedWidth(50.F).HeaderContentPadding(s_ContentHeaderPadding)
						+SHeaderRow::Column(s_ColIdAction).DefaultLabel(LOCTEXT("ResultColAction","Action")).FixedWidth(60.F).HeaderContentPadding(s_ContentHeaderPadding)
						+SHeaderRow::Column(s_ColIdRootImportAsset).DefaultLabel(LOCTEXT("ResultColImportRootAsset","Import Triggering FaceFX Asset")).HeaderContentPadding(s_ContentHeaderPadding)
						+SHeaderRow::Column(s_ColIdImportAsset).DefaultLabel(LOCTEXT("ResultColImportAsset","Import FaceFX Asset")).HeaderContentPadding(s_ContentHeaderPadding)
						+SHeaderRow::Column(s_ColIdAsset).DefaultLabel(LOCTEXT("ResultColAsset","Contextual Asset")).HeaderContentPadding(s_ContentHeaderPadding)
						+SHeaderRow::Column(s_ColIdMessage).DefaultLabel(LOCTEXT("ResultColMessage","Message")).HeaderContentPadding(s_ContentHeaderPadding)
					)
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.OnClicked(this, &FFaceFXResultWidget::OnRollbackChanges)
							.Text(LOCTEXT("ResultActionRollback","Rollback Selected Create Actions"))
						]
					]
				]
			]
		]
	];

	//fill list with entries from result
	const TArray<FFaceFXImportResult>& ResultSetEntries = Args._Result.GetEntries();
	for(const FFaceFXImportResult& ResultSetEntry : ResultSetEntries)
	{
		const TArray<FFaceFXImportActionResult>& ResultEntries = ResultSetEntry.GetEntries();

		for(const FFaceFXImportActionResult& ResultEntry : ResultEntries)
		{
			ListRowEntry* NewEntry = new ListRowEntry();
			NewEntry->ImportRootAsset = ResultSetEntry.GetImportRootAsset();
			NewEntry->Result = ResultEntry;
			ListViewEntries.Add(MakeShareable(NewEntry));
		}
	}

	bWindowTitleSet = false;
	UpdateTitle();
}

TSharedRef<ITableRow> FFaceFXResultWidget::GenerateRow(TSharedPtr<ListRowEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(ListRowWidget, Owner, Entry);
}

bool FFaceFXResultWidget::MergeResult(const FFaceFXImportResultSet& Data)
{
	const TArray<FFaceFXImportResult>& ResultSetEntries = Data.GetEntries();
	if(ResultSetEntries.Num() == 0)
	{
		return true;
	}

	for(const FFaceFXImportResult& ResultSetEntry : ResultSetEntries)
	{
		const TArray<FFaceFXImportActionResult>& ResultEntries = ResultSetEntry.GetEntries();

		for(const FFaceFXImportActionResult& ResultEntry : ResultEntries)
		{
			ListRowEntry* NewEntry = new ListRowEntry();
			NewEntry->ImportRootAsset = ResultSetEntry.GetImportRootAsset();
			NewEntry->Result = ResultEntry;
			ListViewEntries.Add(MakeShareable(NewEntry));
		}
	}

	ListView->RequestListRefresh();

	UpdateTitle();

	return true;
}

void FFaceFXResultWidget::UpdateTitle()
{
	SWindow* Window = FSlateApplication::Get().FindWidgetWindow(AsShared()).Get();
	if(Window)
	{
		if(!bWindowTitleSet)
		{
			WindowTitle = Window->GetTitle();
			bWindowTitleSet = true;
		}

		int32 NumSuccesses = 0;
		int32 NumWarnings = 0;
		int32 NumErrors = 0;

		//locate all errors/warnings
		for(const TSharedPtr<ListRowEntry>& ListViewEntry : ListViewEntries)
		{
			if(const ListRowEntry* Entry = ListViewEntry.Get())
			{
				switch(Entry->Result.GetResultType())
				{
				case FFaceFXImportActionResult::ResultType::Success: ++NumSuccesses; break;
				case FFaceFXImportActionResult::ResultType::Warning: ++NumWarnings; break;
				case FFaceFXImportActionResult::ResultType::Error: ++NumErrors; break;
				}
			}
		}

		Window->SetTitle(FText::Format(LOCTEXT("ResultTitleFormat", "{0}  ({1} Successes, {2} Warnings, {3} Errors)"), WindowTitle,
			FText::FromString(FString::FromInt(NumSuccesses)),
			FText::FromString(FString::FromInt(NumWarnings)),
			FText::FromString(FString::FromInt(NumErrors))));
	}
}

EAppReturnType::Type FFaceFXResultWidget::OpenDialog(const FText& InTitle, bool ShowAsModal)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(InTitle)
		.SizingRule(ESizingRule::UserSized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.ClientSize(FVector2D(1024.F, 800.F));

	Window->SetContent(AsShared());

	//add instance to the open list
	s_OpenInstances.Add(FResultWidgetInstance(Window, AsShared(), InTitle));

	if(ShowAsModal)
	{
		GEditor->EditorAddModalWindow(Window);
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}

	UpdateTitle();

	return EAppReturnType::Ok;
}

EAppReturnType::Type FFaceFXResultWidget::Create(const FText& InTitle, const FFaceFXImportResultSet& ResultSet, bool ShowAsModal, bool MergeWithOpenWindow)
{
	if (!FSlateApplication::IsInitialized())
	{
		//slate less mode (i.e. importasset commandlet)
		return EAppReturnType::Ok;
	}

	if(ResultSet.GetEntries().Num() == 0)
	{
		//nothing to display
		return EAppReturnType::Ok;
	}

	//Check if there is another result widget open for the same type of results. If so merge them. Used to prevent multiple windows opening up after mass imports via drag'n'drop
	if(MergeWithOpenWindow)
	{
		if(FResultWidgetInstance* ExistingWidget = s_OpenInstances.FindByKey(InTitle))
		{
			FFaceFXResultWidget* ExistingResultWidget = ExistingWidget->GetResultWidget();
			if(ExistingResultWidget && ExistingResultWidget->MergeResult(ResultSet))
			{
				return EAppReturnType::Ok;
			}
		}
	}

	auto ResultWidget = SNew(FFaceFXResultWidget).Result(ResultSet);
	return ResultWidget->OpenDialog(InTitle, ShowAsModal);
}

FReply FFaceFXResultWidget::OnRollbackChanges()
{
	//filter out create entries
	TArray<TSharedPtr<ListRowEntry>> SelectedEntries = ListView->GetSelectedItems();
	TArray<TSharedPtr<ListRowEntry>> SelectedCreateEntries;

	for(TSharedPtr<ListRowEntry>& Entry : SelectedEntries)
	{
		if(Entry->Result.CanRollback())
		{
			SelectedCreateEntries.Add(Entry);
		}
	}

	if(SelectedCreateEntries.Num() == 0)
	{
		return FReply::Handled();
	}

	const FText DialogTitle = LOCTEXT("ImportRollbackTitle", "Rollback Changes (Create only)");
	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
		FText::Format(LOCTEXT("ImportRollbackImport", "Are you sure to rollback the changes on the {0} selected create actions ?"), FText::FromString(FString::FromInt(SelectedCreateEntries.Num()))),
		&DialogTitle);

	if(Result == EAppReturnType::Yes)
	{
		int32 RollbackSuccessCount = 0;

		for(TSharedPtr<ListRowEntry>& Entry : SelectedCreateEntries)
		{
			if(Entry->Result.Rollback())
			{
				ListViewEntries.Remove(Entry);
				++RollbackSuccessCount;
			}
		}

		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ImportRollbackImportResult", "{0} out of {1} actions successfully rolled back"),
			FText::FromString(FString::FromInt(RollbackSuccessCount)),
			FText::FromString(FString::FromInt(SelectedCreateEntries.Num()))),
			&DialogTitle);
	}

	return FReply::Handled();
}

void FFaceFXResultWidget::ListRowWidget::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner, TSharedPtr<ListRowEntry> InEntry)
{
	Entry = InEntry;
	SMultiColumnTableRow<TSharedPtr<ListRowEntry>>::Construct(FSuperRowType::FArguments(), InOwner);
}

TSharedRef<SWidget> FFaceFXResultWidget::ListRowWidget::GenerateWidgetForColumn(const FName& ColumnName)
{
	if(ColumnName == s_ColIdRootImportAsset)
	{
		return SNew(FFaceFXAssetRefWidget).AssetRef(Entry->ImportRootAsset.ToSoftObjectPath());
	}
	else if(ColumnName == s_ColIdImportAsset)
	{
		return SNew(FFaceFXAssetRefWidget).AssetRef(Entry->Result.GetImportAsset().ToSoftObjectPath());
	}
	else if(ColumnName == s_ColIdAsset)
	{
		return SNew(FFaceFXAssetRefWidget).AssetRef(Entry->Result.GetAsset().ToSoftObjectPath());
	}
	else if(ColumnName == s_ColIdSuccess)
	{
		static const FSlateBrush* s_BrushSuccess = FFaceFXStyle::GetBrushStateIconSuccess();
		static const FSlateBrush* s_BrushWarning = FFaceFXStyle::GetBrushStateIconWarning();
		static const FSlateBrush* s_BrushError = FFaceFXStyle::GetBrushStateIconError();
		static const FText s_textSuccess = LOCTEXT("ResultTypeSuccess","Success");
		static const FText s_textWarning = LOCTEXT("ResultTypeWarning","Warning");
		static const FText s_textError = LOCTEXT("ResultTypeError","Error");

		const FSlateBrush* Brush = nullptr;
		const FText* Tooltip = nullptr;
		switch(Entry->Result.GetResultType())
		{
		case FFaceFXImportActionResult::ResultType::Success:
			{
				Brush = s_BrushSuccess;
				Tooltip = &s_textSuccess;
				break;
			}
		case FFaceFXImportActionResult::ResultType::Warning:
			{
				Brush = s_BrushWarning;
				Tooltip = &s_textWarning;
				break;
			}
		case FFaceFXImportActionResult::ResultType::Error:
			{
				Brush = s_BrushError;
				Tooltip = &s_textError;
				break;
			}
		}

		return
			SNew(SBox)
			.Padding(s_ContentHeaderPadding)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage).Image(Brush).ToolTip(SNew(SToolTip).Text(Tooltip ? *Tooltip : FText::GetEmpty()))
			];
	}
	else if(ColumnName == s_ColIdAction)
	{
		FText Action;
		switch(Entry->Result.GetType())
		{
		case FFaceFXImportActionResult::ActionType::None: Action = LOCTEXT("ResultActionTypeNone","-"); break;
		case FFaceFXImportActionResult::ActionType::Create :  Action = LOCTEXT("ResultActionTypeCreate","Create"); break;
		case FFaceFXImportActionResult::ActionType::Modify: Action = LOCTEXT("ResultActionTypeModify","Modify"); break;
		}

		return
			SNew(SBox)
			.Padding(s_ContentHeaderPadding)
			[
				SNew(STextBlock).Text(Action)
			];
	}
	else if(ColumnName == s_ColIdMessage)
	{
		const FText& Message = Entry->Result.GetMessage();
		return SNew(SBox)
			.Padding(s_ContentHeaderPadding)
			[
				SNew(STextBlock).Text(Message).ToolTip(SNew(SToolTip).Text(Message))
			];
	}

	return SNew(STextBlock);
}

#undef LOCTEXT_NAMESPACE
