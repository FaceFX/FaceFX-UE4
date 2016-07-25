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

#include "FaceFX.h"
#include "Sequencer/FaceFXAnimationTrack.h"

#include "Sequencer/FaceFXAnimationTrackInstance.h"
#include "Sequencer/FaceFXAnimationSection.h"

#include "IMovieScenePlayer.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXAnimationTrack::UFaceFXAnimationTrack(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(255, 50, 20, 65);
#endif
}

void UFaceFXAnimationTrack::AddSection(float KeyTime, const FFaceFXAnimComponentSet& AnimCompSet)
{
	UFaceFXAnimationSection* NewSection = Cast<UFaceFXAnimationSection>(CreateNewSection());
	{
		NewSection->SetData(AnimCompSet);
		NewSection->InitialPlacement(AnimationSections, KeyTime, KeyTime + NewSection->GetAnimationDuration(), SupportsMultipleRows());
	}
	AddSection(*NewSection);
}

UMovieSceneSection* UFaceFXAnimationTrack::GetSectionAtTime(float Time) const
{
	for (UMovieSceneSection* Section : AnimationSections)
	{
		if (Section->IsActive() && Section->IsTimeWithinSection(Time))
		{
			return Section;
		}
	}
	return nullptr;
}

TSharedPtr<IMovieSceneTrackInstance> UFaceFXAnimationTrack::CreateInstance()
{
	return MakeShareable(new FFaceFXAnimationTrackInstance(this));
}

const TArray<UMovieSceneSection*>& UFaceFXAnimationTrack::GetAllSections() const
{
	return AnimationSections;
}

UMovieSceneSection* UFaceFXAnimationTrack::CreateNewSection()
{
	return NewObject<UFaceFXAnimationSection>(this);
}

void UFaceFXAnimationTrack::RemoveAllAnimationData()
{
	AnimationSections.Empty();
}

bool UFaceFXAnimationTrack::HasSection(const UMovieSceneSection& Section) const
{
	return AnimationSections.Contains(&Section);
}

void UFaceFXAnimationTrack::AddSection(UMovieSceneSection& Section)
{
	AnimationSections.Add(&Section);
}

void UFaceFXAnimationTrack::RemoveSection(UMovieSceneSection& Section)
{
	AnimationSections.Remove(&Section);
}

bool UFaceFXAnimationTrack::IsEmpty() const
{
	return AnimationSections.Num() == 0;
}

TRange<float> UFaceFXAnimationTrack::GetSectionBoundaries() const
{
	TArray<TRange<float>> Bounds;

	for (UMovieSceneSection* Section : AnimationSections)
	{
		Bounds.Add(Section->GetRange());
	}

	return TRange<float>::Hull(Bounds);
}

#if WITH_EDITORONLY_DATA

FText UFaceFXAnimationTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("SequencerTrackName", "FaceFX");
}

#endif //WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE