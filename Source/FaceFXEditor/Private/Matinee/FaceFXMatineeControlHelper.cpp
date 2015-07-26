/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015 OC3 Entertainment, Inc.
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

#include "FaceFXEditor.h"
#include "Matinee/FaceFXMatineeControlHelper.h"
#include "Matinee/FaceFXMatineeControl.h"
#include "Animation/FaceFXComponent.h"
#include "FaceFXCharacter.h"

#include "Editor.h"
#include "SlateCore.h"
#include "SlateBasics.h"
#include "STextEntryPopup.h"
#include "EditorViewportClient.h"
#include "IMatinee.h"
#include "Matinee/MatineeActor.h"
#include "Matinee/InterpGroupInst.h"
#include "EditorModeInterpolation.h"
#include "FaceFXAnim.h"
#include "ContentBrowserModule.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "FaceFX"

/** A single combo box item for the skel mesh selection */
struct FFaceFXSkelMeshSelection
{
	/** The character skel mesh component id */
	FFaceFXSkelMeshComponentId Id;

	/** The text to display on the combo box */
	FString Text;
};

UFaceFXMatineeControlHelper::UFaceFXMatineeControlHelper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), KeyframeAddFaceFXAnim(nullptr), bKeyframeSettingsLoop(false)
{	
}

bool UFaceFXMatineeControlHelper::PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const
{
	FEdModeInterpEdit* Mode = static_cast<FEdModeInterpEdit*>(GLevelEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_InterpEdit ));
	check(Mode);

	IMatineeBase* InterpEd = Mode->InterpEd;
	check(InterpEd);

	UInterpGroupInst* GrInst = InterpEd->GetMatineeActor()->FindFirstGroupInst(Group);
	check(GrInst);

	if (AActor* Actor = GrInst->GetGroupActor())
	{
		//Locate FaceFX component
		if(!Actor->FindComponentByClass<UFaceFXComponent>())
		{
			UE_LOG(LogFaceFX, Warning, TEXT("InterpGroup : FaceFX component missing (%s)"), *Actor->GetName());

			if(bAllowPrompts)
			{
				FNotificationInfo Info(LOCTEXT("MatineeFaceFXMissingComponent", "Unable to add FaceFX Track. Selected actor does not own a FaceFX Component."));
				Info.ExpireDuration = 10.F;
				Info.bUseLargeFont = false;
				Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}

			return false;
		}
	}
	return true;
}

bool UFaceFXMatineeControlHelper::PreCreateKeyframe( UInterpTrack *Track, float fTime ) const
{
	ResetCachedValues();

	UFaceFXMatineeControl* TrackFaceFX = CastChecked<UFaceFXMatineeControl>(Track);
	UInterpGroup* Group = CastChecked<UInterpGroup>(TrackFaceFX->GetOuter());

	AActor* Actor = GetGroupActor(Track);
	if (!Actor)
	{
		// error message
		UE_LOG(LogFaceFX, Warning, TEXT("No Actor is selected. Select actor first."));
		return false;
	}

	UFaceFXComponent* FaceFXComponent = Actor->FindComponentByClass<UFaceFXComponent>();
	if (!FaceFXComponent)
	{
		UE_LOG(LogFaceFX, Warning, TEXT("FaceFX Component isn't found in the selected actor: %s"), *GetNameSafe(Actor));
		return false;
	}

	// Show the dialog.
	FEdModeInterpEdit* Mode = (FEdModeInterpEdit*)GLevelEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_InterpEdit );
	check(Mode);
	check(Mode->InterpEd);

	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if ( Parent.IsValid() )
	{
		FAssetPickerConfig AssetPickerConfig;
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateUObject(this, &UFaceFXMatineeControlHelper::OnAnimAssetSelected, Mode->InterpEd, Track);
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.Filter.ClassNames.Add(UFaceFXAnim::StaticClass()->GetFName());

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		FMenuBuilder MenuBuilder(true, nullptr);

		TSharedPtr<SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>> SkelMeshSelectionComboBox;

		//Add animation specification by using the asset selection
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("MatineeFaceFXKeySettings", "FaceFX Animation Settings"));
		{
			TSharedPtr<SVerticalBox> MenuEntry = 
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(FOnCheckStateChanged::CreateUObject(this, &UFaceFXMatineeControlHelper::OnKeyframeLoopCheckboxChange))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MatineeFaceFXKeySettingsLoop", "Loop Animation"))
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					. BorderImage(FCoreStyle::Get().GetBrush("PopupText.Background"))
					. Padding(10)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("MatineeFaceFXKeySkelMeshSelectionTitle", "Target Skeletal Mesh Component: "))
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(SkelMeshSelectionComboBox, SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>)
							.OptionsSource(&SkelMeshSelectionComboBoxEntries)
							.OnGenerateWidget(SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>::FOnGenerateWidget::CreateUObject(this, &UFaceFXMatineeControlHelper::MakeWidgetFromSkelMeshSelection))
							.OnSelectionChanged(SComboBox<TSharedPtr<FFaceFXSkelMeshSelection>>::FOnSelectionChanged::CreateUObject(this, &UFaceFXMatineeControlHelper::OnSkelMeshComboBoxSelected, Mode->InterpEd, Track))
							[
								SAssignNew(SkelMeshComponentSelection, STextBlock)
								.Text(LOCTEXT("MatineeFaceFXKeySkelMeshSelectionDefaultTitle", "<Default>"))
							]
						]
					]
				];
			MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);

			//Fill Skel mesh component list
			SkelMeshSelectionComboBoxEntries.Reset();
			
			TArray<USkeletalMeshComponent*> SkelMeshComponents;
			FaceFXComponent->GetSetupSkelMeshComponents(SkelMeshComponents);

			//Add default entry
			FFaceFXSkelMeshSelection* NewEntry = new FFaceFXSkelMeshSelection();
			NewEntry->Text = LOCTEXT("MatineeFaceFXKeySkelMeshSelectionDefaultTitle", "<Default>").ToString();
			SkelMeshSelectionComboBoxEntries.Add(MakeShareable(NewEntry));

			for(int32 i=0; i<SkelMeshComponents.Num(); ++i)
			{
				const USkeletalMeshComponent* SkelMeshComp = SkelMeshComponents[i];

				NewEntry = new FFaceFXSkelMeshSelection();
				NewEntry->Id.Index = i;
				NewEntry->Id.Name = SkelMeshComp->GetFName();
				if(SkelMeshComp)
				{
					NewEntry->Text = SkelMeshComp->GetName();
					if(SkelMeshComp->SkeletalMesh)
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
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("MatineeFaceFXKeyAnimID", "Play By FaceFX Animation ID"));

		TSharedPtr<SComboBox<TSharedPtr<FFaceFXAnimId>>> AnimIdComboBox;

		//Add a text input field for the animation ID specifier in case animation linkage is enabled
		TSharedPtr<SVerticalBox> MenuEntry = 
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextEntryPopup)
				.Label(LOCTEXT("MatineeFaceFXKeyAnimGroupTitle", "Animation Group"))
				.OnTextCommitted(FOnTextCommitted::CreateUObject(this, &UFaceFXMatineeControlHelper::OnAnimGroupCommitted, Mode->InterpEd, Track))
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextEntryPopup)
				.Label(LOCTEXT("MatineeFaceFXKeyAnimIDTitle", "Animation Id"))
				.OnTextCommitted(FOnTextCommitted::CreateUObject(this, &UFaceFXMatineeControlHelper::OnAnimIdCommitted, Mode->InterpEd, Track))
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				. BorderImage(FCoreStyle::Get().GetBrush("PopupText.Background"))
				. Padding(10)
				[
					SAssignNew(AnimIdComboBox, SComboBox<TSharedPtr<FFaceFXAnimId>>)
					.OptionsSource(&KeyframeAddFaceDXExistingAnimIds)
					.OnGenerateWidget(SComboBox<TSharedPtr<FFaceFXAnimId>>::FOnGenerateWidget::CreateUObject(this, &UFaceFXMatineeControlHelper::MakeWidgetFromAnimId))
					.OnSelectionChanged(SComboBox<TSharedPtr<FFaceFXAnimId>>::FOnSelectionChanged::CreateUObject(this, &UFaceFXMatineeControlHelper::OnAnimIdComboBoxSelected, Mode->InterpEd, Track))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MatineeFaceFXKeyAnimIDExistTitle", "Select Existing Animation ID"))
					]
				]
			];

		MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);

		MenuBuilder.EndSection();

		//fill existing animation ids
		KeyframeAddFaceDXExistingAnimIds.Reset();
		if(const UFaceFXCharacter* Character = FaceFXComponent->GetCharacter())
		{
			TArray<FFaceFXAnimId> ExistingAnimIds;
			if(Character->GetAllLinkedAnimationIds(ExistingAnimIds))
			{
				for(const FFaceFXAnimId& AnimId : ExistingAnimIds)
				{
					KeyframeAddFaceDXExistingAnimIds.Add(MakeShareable(new FFaceFXAnimId(AnimId)));
				}
			}
		}
		AnimIdComboBox->RefreshOptions();

#endif //FACEFX_USEANIMATIONLINKAGE

		//Add animation specification by using the asset selection
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("MatineeFaceFXKeyAnimPicker", "Play By FaceFX Animation Asset"));
		{
			TSharedPtr<SBox> MenuEntry = SNew(SBox)
			.WidthOverride(300.0f)
			.HeightOverride(300.f)
			[
				ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
			];
			MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
		}
		MenuBuilder.EndSection();

		EntryPopupWindow = FSlateApplication::Get().PushMenu(Parent.ToSharedRef(), MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}

	return false;
}

void UFaceFXMatineeControlHelper::OnAnimAssetSelected(const FAssetData& AssetData, IMatineeBase* Matinee, UInterpTrack* Track)
{
	if( EntryPopupWindow.IsValid() )
	{
		EntryPopupWindow.Pin()->RequestDestroyWindow();
	}

	if (UFaceFXAnim* SelectedAnim = Cast<UFaceFXAnim>(AssetData.GetAsset()))
	{
		KeyframeAddFaceFXAnim = SelectedAnim;
		Matinee->FinishAddKey(Track, true);
	}
}

void UFaceFXMatineeControlHelper::OnKeyframeLoopCheckboxChange(ECheckBoxState NewState)
{
	bKeyframeSettingsLoop = NewState == ECheckBoxState::Checked;
}

void UFaceFXMatineeControlHelper::OnSkelMeshComboBoxSelected(TSharedPtr<FFaceFXSkelMeshSelection> NewSelection, enum ESelectInfo::Type SelectInfo, class IMatineeBase* Matinee, UInterpTrack* Track)
{
	if(const FFaceFXSkelMeshSelection* Selection = NewSelection.Get())
	{
		KeyframeAddSkelMeshComponentId = Selection->Id;
		SkelMeshComponentSelection->SetText(Selection->Text);
	}
}

TSharedRef<SWidget> UFaceFXMatineeControlHelper::MakeWidgetFromSkelMeshSelection(TSharedPtr<struct FFaceFXSkelMeshSelection> InItem)
{
	FString Text;
	if(const FFaceFXSkelMeshSelection* SkelMeshSelection = InItem.Get())
	{
		Text = SkelMeshSelection->Text;
	}

	return SNew(STextBlock).Text(FText::FromString(*Text));
}

#if FACEFX_USEANIMATIONLINKAGE

void UFaceFXMatineeControlHelper::OnAnimGroupCommitted(const FText& Text, ETextCommit::Type Type, IMatineeBase* Matinee, UInterpTrack* Track)
{
	check(Matinee);

	KeyframeAddFaceFXAnimId.Group = FName(*Text.ToString());
	
	//if any of the two (group & id) popup widgets commit their text, we assume the input is done
	if(Type != ETextCommit::OnEnter)
	{
		return;
	}

	if( EntryPopupWindow.IsValid() )
	{
		EntryPopupWindow.Pin()->RequestDestroyWindow();
	}

	Matinee->FinishAddKey(Track, true);
}

void UFaceFXMatineeControlHelper::OnAnimIdCommitted(const FText& Text, ETextCommit::Type Type, IMatineeBase* Matinee, UInterpTrack* Track)
{
	check(Matinee);

	KeyframeAddFaceFXAnimId.Name = FName(*Text.ToString());
	
	//if any of the two (group & id) popup widgets commit their text, we assume the input is done
	if(Type != ETextCommit::OnEnter)
	{
		return;
	}

	if( EntryPopupWindow.IsValid() )
	{
		EntryPopupWindow.Pin()->RequestDestroyWindow();
	}

	Matinee->FinishAddKey(Track, true);
}

TSharedRef<SWidget> UFaceFXMatineeControlHelper::MakeWidgetFromAnimId(TSharedPtr<FFaceFXAnimId> InItem)
{
	FString Text;
	if(FFaceFXAnimId* AnimId = InItem.Get())
	{
		Text = AnimId->Group.IsNone() ? TEXT("") : AnimId->Group.ToString() + TEXT(" / ");
		Text += AnimId->Name.ToString();
	}

	return SNew(STextBlock).Text(FText::FromString(*Text));
}

void UFaceFXMatineeControlHelper::OnAnimIdComboBoxSelected(TSharedPtr<FFaceFXAnimId> NewSelection, enum ESelectInfo::Type SelectInfo, IMatineeBase* Matinee, UInterpTrack* Track)
{
	check(Matinee);

	if( EntryPopupWindow.IsValid() )
	{
		EntryPopupWindow.Pin()->RequestDestroyWindow();
	}

	if(const FFaceFXAnimId* SelectedAnimId = NewSelection.Get())
	{
		KeyframeAddFaceFXAnimId = *SelectedAnimId;
	}
	else
	{
		KeyframeAddFaceFXAnimId.Reset();
	}

	Matinee->FinishAddKey(Track, true);
};

#endif //FACEFX_USEANIMATIONLINKAGE

void UFaceFXMatineeControlHelper::PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const
{
	UFaceFXMatineeControl* TrackFaceFX = CastChecked<UFaceFXMatineeControl>(Track);
	FFaceFXTrackKey& NewAnimKey = TrackFaceFX->Keys[KeyIndex];
	NewAnimKey.SkelMeshComponentId = KeyframeAddSkelMeshComponentId;
	NewAnimKey.Animation = KeyframeAddFaceFXAnim;
#if FACEFX_USEANIMATIONLINKAGE
	NewAnimKey.AnimationId = KeyframeAddFaceFXAnimId;
#endif //FACEFX_USEANIMATIONLINKAGE
	NewAnimKey.bLoop = bKeyframeSettingsLoop;	

	ResetCachedValues();
}

#undef LOCTEXT_NAMESPACE