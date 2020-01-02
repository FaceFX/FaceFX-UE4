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

#include "FaceFXAnimationKeyDetailsDialog.h"
#include "FaceFXEditor.h"
#include "FaceFXCharacter.h"
#include "FaceFXAnim.h"
#include "Animation/FaceFXComponent.h"
#include "Editor.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "EditorModeInterpolation.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "FaceFX"

/** A single combo box item for the skel mesh selection */
struct FFaceFXSkelMeshSelection
{
	/** The character skel mesh component id */
	FFaceFXSkelMeshComponentId Id;

	/** The text to display on the combo box */
	FString Text;
};

void FFaceFXAnimationKeyDetailsDialog::CloseDialog(bool bCanceled, bool bTriggerOnlyOnWindowExist)
{
	const bool triggerEvent = !bTriggerOnlyOnWindowExist || EntryPopupMenu.IsValid();

	if (EntryPopupMenu.IsValid())
	{
		bIsCancelled = bCanceled;

		EntryPopupMenu.Pin()->Dismiss();
		EntryPopupMenu.Reset();
	}

	if (triggerEvent)
	{
		OnDialogCloseDelegate.ExecuteIfBound();
	}
}

TSharedRef<SWidget> FFaceFXAnimationKeyDetailsDialog::CreateWidget(UFaceFXComponent* FaceFXComponent, const FSimpleDelegate& OnCloseDelegeate)
{
	Reset();

	OnDialogCloseDelegate = OnCloseDelegeate;

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnAnimAssetSelected);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.Filter.ClassNames.Add(UFaceFXAnim::StaticClass()->GetFName());

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FMenuBuilder MenuBuilder(true, nullptr);

	TSharedPtr<SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>> SkelMeshSelectionComboBox;

	//Add animation specification by using the asset selection
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("FaceFXKeySettings", "FaceFX Animation Settings"));
	{
		TSharedPtr<SVerticalBox> MenuEntrySettings =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("PopupText.Background"))
				.Padding(10)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("FaceFXKeySkelMeshSelectionTitle", "Target Skeletal Mesh Component: "))
					]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(SkelMeshSelectionComboBox, SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>)
							.OptionsSource(&SkelMeshSelectionComboBoxEntries)
							.OnGenerateWidget(SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>::FOnGenerateWidget::CreateStatic(&FFaceFXAnimationKeyDetailsDialog::MakeWidgetFromSkelMeshSelection))
							.OnSelectionChanged(SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>::FOnSelectionChanged::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnSkelMeshComboBoxSelected))
							[
								SAssignNew(SkelMeshComponentSelection, STextBlock)
								.Text(LOCTEXT("FaceFXKeySkelMeshSelectionDefaultTitle", "<Default>"))
							]
						]
				]
			];

		if (bIsShowLooping)
		{
			MenuEntrySettings->InsertSlot(0)
				.AutoHeight()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(FOnCheckStateChanged::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnKeyframeLoopCheckboxChange))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("FaceFXKeySettingsLoop", "Loop Animation"))
					]
				];
		}

		MenuBuilder.AddWidget(MenuEntrySettings.ToSharedRef(), FText::GetEmpty(), true);

		//Fill Skel mesh component list
		SkelMeshSelectionComboBoxEntries.Reset();

		TArray<USkeletalMeshComponent*> SkelMeshComponents;
		FaceFXComponent->GetSetupSkelMeshComponents(SkelMeshComponents);

		//Add default entry
		FFaceFXSkelMeshSelection* NewEntry = new FFaceFXSkelMeshSelection();
		NewEntry->Text = LOCTEXT("FaceFXKeySkelMeshSelectionDefaultTitle", "<Default>").ToString();
		SkelMeshSelectionComboBoxEntries.Add(MakeShareable(NewEntry));

		for (int32 i = 0; i < SkelMeshComponents.Num(); ++i)
		{
			const USkeletalMeshComponent* SkelMeshComp = SkelMeshComponents[i];

			NewEntry = new FFaceFXSkelMeshSelection();
			NewEntry->Id.Index = i;
			NewEntry->Id.Name = SkelMeshComp->GetFName();
			if (SkelMeshComp)
			{
				NewEntry->Text = SkelMeshComp->GetName();
				if (SkelMeshComp->SkeletalMesh)
				{
					NewEntry->Text += TEXT("  [Mesh: ") + SkelMeshComp->SkeletalMesh->GetName() + TEXT("]");
				}
			}
			else
			{
				NewEntry->Text = TEXT("Unknown");
			}

			SkelMeshSelectionComboBoxEntries.Add(MakeShareable(NewEntry));
		}
		SkelMeshSelectionComboBox->RefreshOptions();
	}
	MenuBuilder.EndSection();

#if FACEFX_USEANIMATIONLINKAGE

	//Add animation specification by using the animation ID
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("FaceFXKeyAnimID", "Play By FaceFX Animation ID"));

	TSharedPtr<SComboBox<TSharedPtr<FFaceFXAnimId>>> AnimIdComboBox;

	//Add a text input field for the animation ID specifier in case animation linkage is enabled
	TSharedPtr<SVerticalBox> MenuEntryAnimId =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextEntryPopup)
			.Label(LOCTEXT("FaceFXKeyAnimGroupTitle", "Animation Group"))
			.OnTextCommitted(FOnTextCommitted::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnAnimGroupCommitted))
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextEntryPopup)
			.Label(LOCTEXT("FaceFXKeyAnimIDTitle", "Animation Id"))
			.OnTextCommitted(FOnTextCommitted::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnAnimIdCommitted))
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("PopupText.Background"))
			.Padding(10)
			[
				SAssignNew(AnimIdComboBox, SComboBox<TSharedPtr<FFaceFXAnimId>>)
				.OptionsSource(&ExistingAnimIds)
				.OnGenerateWidget(SComboBox<TSharedPtr<FFaceFXAnimId>>::FOnGenerateWidget::CreateStatic(&FFaceFXAnimationKeyDetailsDialog::MakeWidgetFromAnimId))
				.OnSelectionChanged(SComboBox<TSharedPtr<FFaceFXAnimId>>::FOnSelectionChanged::CreateRaw(this, &FFaceFXAnimationKeyDetailsDialog::OnAnimIdComboBoxSelected))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FaceFXKeyAnimIDExistTitle", "Select Existing Animation ID"))
				]
			]
		];

	MenuBuilder.AddWidget(MenuEntryAnimId.ToSharedRef(), FText::GetEmpty(), true);

	MenuBuilder.EndSection();

	//fill existing animation ids
	ExistingAnimIds.Reset();
	if (const UFaceFXCharacter* Character = FaceFXComponent->GetCharacter())
	{
		TArray<FFaceFXAnimId> AllLinkedAnimIds;
		if (Character->GetAllLinkedAnimationIds(AllLinkedAnimIds))
		{
			for (const FFaceFXAnimId& AnimId : AllLinkedAnimIds)
			{
				ExistingAnimIds.Add(MakeShareable(new FFaceFXAnimId(AnimId)));
			}
		}
	}
	AnimIdComboBox->RefreshOptions();

#endif //FACEFX_USEANIMATIONLINKAGE

	//Add animation specification by using the asset selection
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("FaceFXKeyAnimPicker", "Play By FaceFX Animation Asset"));
	{
		TSharedPtr<SBox> MenuEntryAnimAsset = SNew(SBox)
			.WidthOverride(300.0f)
			.HeightOverride(300.f)
			[
				ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
			];
		MenuBuilder.AddWidget(MenuEntryAnimAsset.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

bool FFaceFXAnimationKeyDetailsDialog::ShowDialog(UFaceFXComponent* FaceFXComponent, const FSimpleDelegate& OnCloseDelegeate)
{
	Reset();
	CloseDialog(true, true);

	TSharedPtr< SWindow > Parent;

    TArray<TSharedRef<SWindow> > AllWindows;
    FSlateApplication::Get().GetAllVisibleWindowsOrdered(AllWindows);

    if (AllWindows.Num() != 0)
    {
        Parent = AllWindows[0];
    }

	if (Parent.IsValid())
	{
		EntryPopupMenu = FSlateApplication::Get().PushMenu(Parent.ToSharedRef(), FWidgetPath(), CreateWidget(FaceFXComponent, OnCloseDelegeate), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}

  	return EntryPopupMenu.IsValid();
}

TSharedRef<SWidget> FFaceFXAnimationKeyDetailsDialog::MakeWidgetFromAnimId(TSharedPtr<FFaceFXAnimId> InItem)
{
	FString Text;
	if (FFaceFXAnimId* AnimId = InItem.Get())
	{
		Text = AnimId->Group.IsNone() ? TEXT("") : AnimId->Group.ToString() + TEXT(" / ");
		Text += AnimId->Name.ToString();
	}

	return SNew(STextBlock).Text(FText::FromString(*Text));
}

TSharedRef<SWidget> FFaceFXAnimationKeyDetailsDialog::MakeWidgetFromSkelMeshSelection(TSharedPtr<FFaceFXSkelMeshSelection> InItem)
{
	FString Text;
	if (const FFaceFXSkelMeshSelection* SkelMeshSelection = InItem.Get())
	{
		Text = SkelMeshSelection->Text;
	}

	return SNew(STextBlock).Text(FText::FromString(*Text));
}

void FFaceFXAnimationKeyDetailsDialog::OnSkelMeshComboBoxSelected(TSharedPtr<FFaceFXSkelMeshSelection> NewSelection, enum ESelectInfo::Type SelectInfo)
{
	if (const FFaceFXSkelMeshSelection* Selection = NewSelection.Get())
	{
		SelectionData.SkelMeshComponentId = Selection->Id;
		SkelMeshComponentSelection->SetText(FText::FromString(Selection->Text));
	}
}

void FFaceFXAnimationKeyDetailsDialog::OnKeyframeLoopCheckboxChange(ECheckBoxState NewState)
{
	bIsLooping = NewState == ECheckBoxState::Checked;
}

void FFaceFXAnimationKeyDetailsDialog::OnAnimAssetSelected(const FAssetData& AssetData)
{
	SelectionData.AnimationId.Reset();

	bool bCancel = true;
	if (UFaceFXAnim* AssetDataAnim = Cast<UFaceFXAnim>(AssetData.GetAsset()))
	{
		SelectionData.Animation = AssetDataAnim;
		bCancel = false;
	}

	CloseDialog(bCancel);
}

#if FACEFX_USEANIMATIONLINKAGE

void FFaceFXAnimationKeyDetailsDialog::OnAnimGroupCommitted(const FText& Text, ETextCommit::Type Type)
{
	SelectionData.AnimationId.Group = FName(*Text.ToString());

	//if any of the two (group & id) popup widgets commit their text, we assume the input is done
	if (Type != ETextCommit::OnEnter)
	{
		return;
	}

	CloseDialog(false);
}

void FFaceFXAnimationKeyDetailsDialog::OnAnimIdCommitted(const FText& Text, ETextCommit::Type Type)
{
	SelectionData.AnimationId.Name = FName(*Text.ToString());

	//if any of the two (group & id) popup widgets commit their text, we assume the input is done
	if (Type != ETextCommit::OnEnter)
	{
		return;
	}

	CloseDialog(false);
}

void FFaceFXAnimationKeyDetailsDialog::OnAnimIdComboBoxSelected(TSharedPtr<FFaceFXAnimId> NewSelection, enum ESelectInfo::Type SelectInfo)
{
	bool bCancel = true;
	if (const FFaceFXAnimId* NewSelectionPtr = NewSelection.Get())
	{
		SelectionData.AnimationId = *NewSelectionPtr;
		bCancel = false;
	}
	else
	{
		SelectionData.AnimationId.Reset();
	}

	CloseDialog(bCancel);
}

#endif //FACEFX_USEANIMATIONLINKAGE

#undef LOCTEXT_NAMESPACE
