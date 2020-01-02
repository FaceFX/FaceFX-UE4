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

#include "CoreMinimal.h"
#include "FaceFXEditorTools.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/SWindow.h"

/** A widget that wraps a asset reference into a textbox and goto button */
class FFaceFXAssetRefWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(FFaceFXAssetRefWidget){}
	SLATE_ARGUMENT(FSoftObjectPath, AssetRef)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:

	FReply OnClicked(FSoftObjectPath InAssetRef);
};

/** A widget that shows the Import result data */
class FFaceFXResultWidget : public SCompoundWidget
{
	struct ListRowEntry
	{
		FFaceFXImportActionResult Result;

		/** The root action that initially requested the import */
		TSoftObjectPtr<class UFaceFXAsset> ImportRootAsset;
	};

	/** Widget used to display the table rows */
	class ListRowWidget : public SMultiColumnTableRow< TSharedPtr<ListRowEntry> >
	{
	public:
		SLATE_BEGIN_ARGS(ListRowWidget){}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner, TSharedPtr<ListRowEntry> InEntry);
		TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName);

	private:

		/** The data entry associated with this row */
		TSharedPtr<ListRowEntry> Entry;
	};

public:
	SLATE_BEGIN_ARGS(FFaceFXResultWidget) {}
	SLATE_ARGUMENT(FFaceFXImportResultSet, Result)
	SLATE_END_ARGS()

	virtual ~FFaceFXResultWidget();

	void Construct(const FArguments& Args);

	/**
	* Merges the given data into the current result set
	* @param Data The data to merge
	* @returns True if succeeded, else false
	*/
	bool MergeResult(const FFaceFXImportResultSet& Data);

	/**
	* Opens this widget in a modal window
	* @param InTitle The window title
	* @param ShowAsModal Indicator if this widget shall be displayed in a modal window
	*/
	EAppReturnType::Type OpenDialog(const FText& InTitle, bool ShowAsModal = false);

	/**
	* Creates a result dialog
	* @param InTitle The window title
	* @param ResultSet The result data
	* @param ShowAsModal Indicator if this widget shall be displayed in a modal window
	* @param MergeWithOpenWindow Indicator if the result set shall be merged into any, currently open result window
	*/
	static EAppReturnType::Type Create(const FText& InTitle, const FFaceFXImportResultSet& ResultSet, bool ShowAsModal = false, bool MergeWithOpenWindow = true);

private:

	/** Represents a result widget instance. Might be still open or not */
	struct FResultWidgetInstance
	{
		FResultWidgetInstance(const TWeakPtr<class SWidget>& InWindow, const TWeakPtr<SWidget>& InWidget, const FText& InTitle) : Window(InWindow), Widget(InWidget), Title(InTitle){}

		/**
		* Gets the indicator if this is a valid widget instance
		* @returns True if valid, else false
		*/
		inline bool IsValid() const
		{
			return Window.IsValid() && Widget.IsValid();
		}

		/**
		* Gets the result widget
		* @returns The result widget
		*/
		inline class SWindow* GetWindow() const
		{
			SWidget* ResultWidget = Window.Pin().Get();
			return ResultWidget ? static_cast<SWindow*>(ResultWidget) : nullptr;
		}

		/**
		* Gets the result widget
		* @returns The result widget
		*/
		inline FFaceFXResultWidget* GetResultWidget() const
		{
			SWidget* ResultWidget = Widget.Pin().Get();
			return ResultWidget ? static_cast<FFaceFXResultWidget*>(ResultWidget) : nullptr;
		}

		FORCEINLINE bool operator==(const FText& InTitle) const
		{
			return Title.EqualTo(InTitle);
		}

		FORCEINLINE bool operator==(const class SWidget* InWidget) const
		{
			return Widget.Pin().Get() == InWidget;
		}

	//private:

		/** The owning window */
		TWeakPtr<class SWidget> Window;

		/** The widget */
		TWeakPtr<class SWidget> Widget;

		/** The original title */
		FText Title;
	};

	/** Updates the title of the window */
	void UpdateTitle();

	/** Event handler for when the user requested a rollback of the selected changes */
	FReply OnRollbackChanges();

	/**  The list view widget */
	TSharedPtr<SListView<TSharedPtr<ListRowEntry>>> ListView;

	/** The list view entries to display */
	TArray<TSharedPtr<ListRowEntry>> ListViewEntries;

	/** Callback for when the list view requests a new row widget */
	TSharedRef<ITableRow> GenerateRow(TSharedPtr<ListRowEntry> Entry, const TSharedRef<STableViewBase>& Owner);

	/** The original title of the owning window */
	FText WindowTitle;

	/** Indicator if the window title was retrieved already */
	uint8 bWindowTitleSet : 1;

	/** The list of open instances */
	static TArray<FResultWidgetInstance> s_OpenInstances;
};
