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

#include "Sequencer/FaceFXAnimationTrackEditor.h"
#include "FaceFXEditor.h"
#include "Sequencer/FaceFXAnimationTrack.h"
#include "Sequencer/FaceFXAnimationSection.h"
#include "Animation/FaceFXComponent.h"
#include "Include/Slate/FaceFXAnimationKeyDetailsDialog.h"
#include "FaceFXAnim.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieSceneTrack.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerSection.h"
#include "SequencerUtilities.h"
#include "SequencerSectionPainter.h"
#include "CommonMovieSceneTools.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "FaceFX"

TWeakPtr<ISequencer> FFaceFXAnimationTrackEditor::CurrentSequencer;

UMovieSceneSection* FFaceFXAnimationSection::GetSectionObject()
{
	return &Section;
}

FText FFaceFXAnimationSection::GetSectionTitle() const
{
	if (const UFaceFXAnimationSection* AnimSection = Cast<UFaceFXAnimationSection>(&Section))
	{
		return AnimSection->GetTitle();
	}
	return LOCTEXT("SequencerAnimationSectionMissingTitle", "<Missing Title>");
}

float FFaceFXAnimationSection::GetSectionHeight() const
{
	return 20.F;
}

int32 FFaceFXAnimationSection::OnPaintSection(FSequencerSectionPainter& Painter) const
{
	const UFaceFXAnimationSection* AnimSection = CastChecked<UFaceFXAnimationSection>(&Section);
	int32 LayerId = Painter.PaintSectionBackground();

	if (!AnimSection->HasStartFrame())
	{
		return LayerId;
	}

	const UFaceFXAnimationTrack* AnimTrack = AnimSection->GetTrack();
	if (!AnimTrack)
	{
		return LayerId;
	}

	const UMovieScene* TrackMovieScene = AnimTrack->GetTypedOuter<UMovieScene>();
	if (!TrackMovieScene)
	{
		return LayerId;
	}

	const ESlateDrawEffect DrawEffects = Painter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const float AnimSectionDuration = AnimSection->GetAnimationDurationInSeconds();

	const FTimeToPixel& TimeToPixelConverter = Painter.GetTimeConverter();

	// Add lines where the animation starts and ends/loops
	const FFrameNumber StartFrame = AnimSection->GetInclusiveStartFrame();
	const FFrameNumber EndFrame = AnimSection->HasEndFrame() ? AnimSection->GetExclusiveEndFrame() : FFrameNumber(TNumericLimits<FFrameNumber>::Max());

	//get the animation length in frames
	const FFrameRate FrameResolution = TrackMovieScene->GetTickResolution();
	const float AnimLength = AnimSectionDuration - AnimSection->GetStartOffset() + AnimSection->GetEndOffset();
	const FFrameNumber AnimLengthFrames = FrameResolution.AsFrameNumber(AnimLength);

	//go over all full loops of the animation and draw the markers between each of them
	FFrameNumber CurrentFrame = StartFrame;
	while (CurrentFrame < EndFrame && !FMath::IsNearlyZero(AnimSectionDuration) && AnimLength > 0)
	{
		if (CurrentFrame > StartFrame)
		{
			const float CurrentPixels = TimeToPixelConverter.FrameToPixel(CurrentFrame);

			TArray<FVector2D> Points;
			Points.Add(FVector2D(CurrentPixels, 0));
			Points.Add(FVector2D(CurrentPixels, Painter.SectionGeometry.Size.Y));

			FSlateDrawElement::MakeLines(Painter.DrawElements, ++LayerId, Painter.SectionGeometry.ToPaintGeometry(), Points, DrawEffects);
		}
		CurrentFrame += AnimLengthFrames;
	}
	return LayerId;
}

TSharedRef<ISequencerTrackEditor> FFaceFXAnimationTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	CurrentSequencer = InSequencer;
	return MakeShareable(new FFaceFXAnimationTrackEditor(InSequencer));
}

bool FFaceFXAnimationTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UFaceFXAnimationTrack::StaticClass();
}

TSharedRef<ISequencerSection> FFaceFXAnimationTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	check(SupportsType(SectionObject.GetOuter()->GetClass()));
	return MakeShareable(new FFaceFXAnimationSection(SectionObject));
}

void FFaceFXAnimationTrackEditor::AddKey(const FGuid& ObjectGuid)
{
	if (UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(ObjectGuid))
	{
		KeyDialog.ShowDialog(FaceFXComponent, FSimpleDelegate::CreateRaw(this, &FFaceFXAnimationTrackEditor::OnFaceFXTrackDialogClosed, ObjectGuid));
	}
}

void FFaceFXAnimationTrackEditor::OnFaceFXTrackDialogClosed(FGuid ObjectBinding)
{
	TSharedPtr<ISequencer> LocalSequencer = GetSequencer();

	if (LocalSequencer.IsValid())
	{
		UObject* Object = LocalSequencer->FindSpawnedObjectOrTemplate(ObjectBinding);
		AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FFaceFXAnimationTrackEditor::AddFaceFXSection, Object, KeyDialog.GetSelectedAnimSet()));
	}
}

void FFaceFXAnimationTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
	if (ObjectClass != UFaceFXComponent::StaticClass())
	{
		//only show widget on the component menu
		return;
	}

	if (UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(ObjectBindings[0]))
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SequencerAddSection", "Facial Animation"),
			LOCTEXT("SequencerAddSectionTooltip", "Adds a FaceFX facial animation section"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FFaceFXAnimationTrackEditor::OnAddKey, ObjectBindings[0])));
	}
}

TSharedRef<SWidget> FFaceFXAnimationTrackEditor::CreateOutlinerWidget(FGuid ObjectBinding)
{
	if (UFaceFXComponent* FaceFXComponent = GetFaceFXComponent(ObjectBinding))
	{
		return KeyDialog.CreateWidget(FaceFXComponent, FSimpleDelegate::CreateRaw(this, &FFaceFXAnimationTrackEditor::OnFaceFXTrackDialogClosed, ObjectBinding));
	}

	return SNew(STextBlock).Text(LOCTEXT("SequencerAddSectionMissingComp", "Missing FaceFX component."));
}

FKeyPropertyResult FFaceFXAnimationTrackEditor::AddFaceFXSection(FFrameNumber KeyTime, UObject* Object, FFaceFXAnimComponentSet AnimCompSet)
{
	FKeyPropertyResult result;

	bool bHandleCreated = false;
	bool bTrackCreated = false;
	bool bTrackModified = false;

	FFindOrCreateHandleResult HandleResult = FindOrCreateHandleToObject(Object);
	FGuid ObjectHandle = HandleResult.Handle;
	result.bHandleCreated |= HandleResult.bWasCreated;
	if (ObjectHandle.IsValid())
	{
		FFindOrCreateTrackResult TrackResult = FindOrCreateTrackForObject(ObjectHandle, UFaceFXAnimationTrack::StaticClass());
		UMovieSceneTrack* Track = TrackResult.Track;
		result.bTrackCreated |= TrackResult.bWasCreated;

		if (UFaceFXAnimationTrack* AnimTrack = Cast<UFaceFXAnimationTrack>(Track))
		{
			AnimTrack->AddSection(KeyTime, AnimCompSet);
			result.bTrackModified = true;
		}
	}

	return result;
}

UFaceFXComponent* FFaceFXAnimationTrackEditor::GetFaceFXComponent(const FGuid& Guid)
{
	TSharedPtr<ISequencer> LocalSequencer = GetSequencer();
	UObject* BoundObject = LocalSequencer.IsValid() ? LocalSequencer->FindSpawnedObjectOrTemplate(Guid) : nullptr;

	if (AActor* Actor = Cast<AActor>(BoundObject))
	{
		return Actor->FindComponentByClass<UFaceFXComponent>();
	}
	//Fallback to direct cast
	return Cast<UFaceFXComponent>(BoundObject);
}

TSharedPtr<SWidget> FFaceFXAnimationTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	if (GetFaceFXComponent(ObjectBinding))
	{
		//only create widgets for valid bindings

		TSharedRef<SWidget> FaceFXAddTrackButton = FSequencerUtilities::MakeAddButton(LOCTEXT("SequencerAddSection", "Facial Animation"),
			FOnGetContent::CreateSP(this, &FFaceFXAnimationTrackEditor::CreateOutlinerWidget, ObjectBinding), Params.NodeIsHovered, GetSequencer());
		FaceFXAddTrackButton->SetToolTipText(LOCTEXT("SequencerAddSectionTooltip", "Adds a FaceFX facial animation section"));

		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				FaceFXAddTrackButton
			];
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
