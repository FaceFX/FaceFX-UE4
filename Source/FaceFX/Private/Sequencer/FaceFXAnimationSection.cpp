/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2017 OC3 Entertainment, Inc.
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

#include "Sequencer/FaceFXAnimationSection.h"
#include "FaceFX.h"

#include "Sequencer/FaceFXAnimationTrack.h"
#include "FaceFXCharacter.h"

#include "MovieSceneSequence.h"
#include "MovieScenePossessable.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXAnimationSection::UFaceFXAnimationSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StartOffset = 0.f;
	EndOffset = 0.f;

	Time = 0.F;
	bIsAnimationDurationLoaded = false;
	AnimationDuration = 0.F;
}

void UFaceFXAnimationSection::TrimSection(float TrimTime, bool bTrimLeft)
{
	if (IsTimeWithinSection(TrimTime))
	{
		SetFlags(RF_Transactional);
		if (TryModify())
		{
			if (bTrimLeft)
			{
				StartOffset = TrimTime - GetStartTime();
				SetStartTime(TrimTime);
			}
			else
			{
				SetEndTime(TrimTime);
			}
		}
	}
}

UMovieSceneSection* UFaceFXAnimationSection::SplitSection(float SplitTime)
{
	const float AnimPosition = SplitTime - GetStartTime();
	const float AnimLength = GetAnimationDuration() - GetStartOffset() + GetEndOffset();
	const float NewOffset = FMath::Fmod(AnimPosition, AnimLength) + GetStartOffset();

	UMovieSceneSection* NewSection = Super::SplitSection(SplitTime);
	if (UFaceFXAnimationSection* NewAnimSection = Cast<UFaceFXAnimationSection>(NewSection))
	{
		NewAnimSection->SetStartOffset(NewOffset);
	}
	return NewSection;
}

void UFaceFXAnimationSection::GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const
{
	Super::GetSnapTimes(OutSnapTimes, bGetSectionBorders);

	const float CurrentDuration = GetAnimationDuration();
	if (FMath::IsNearlyZero(CurrentDuration))
	{
		//nothing to snap
		return;
	}

	float CurrentTime = GetStartTime();
	const float AnimLength = (CurrentDuration - (GetStartOffset() + GetEndOffset()));

	// Snap to the repeat times
	while (CurrentTime <= GetEndTime() && AnimLength > 0)
	{
		if (CurrentTime >= GetStartTime())
		{
			OutSnapTimes.Add(CurrentTime);
		}
		CurrentTime += AnimLength;
	}
}

UFaceFXAnim* UFaceFXAnimationSection::GetAnimation(const TAssetPtr<UFaceFXAnim>& Asset, UObject* Owner)
{
	UFaceFXAnim* NewAnim = Asset.Get();
	if (!NewAnim && Asset.ToStringReference().IsValid())
	{
		NewAnim = Cast<UFaceFXAnim>(StaticLoadObject(UFaceFXAnim::StaticClass(), Owner, *Asset.ToStringReference().ToString()));
	}
	return NewAnim;
}

float UFaceFXAnimationSection::GetAnimationDuration(const AActor* Actor) const
{
	if (!bIsAnimationDurationLoaded)
	{
#if FACEFX_USEANIMATIONLINKAGE
		//try to resolve the animation duration
		if (AnimationId.IsValid())
		{
			if (!Actor)
			{
				//fetch from track owner
				Actor = GetActor();
			}
			//resolve by animation id
			float AnimStart, AnimEnd;
			if (UFaceFXCharacter::GetAnimationBoundsById(Actor, AnimationId, AnimStart, AnimEnd))
			{
				AnimationDuration = AnimEnd - AnimStart;
			}
		}
		else
#endif //FACEFX_USEANIMATIONLINKAGE
		if (UFaceFXAnim* TargetAnimation = GetAnimation())
		{
			//resolve by animation asset
			float AnimStart, AnimEnd;
			if (UFaceFXCharacter::GetAnimationBounds(TargetAnimation, AnimStart, AnimEnd))
			{
				AnimationDuration = AnimEnd - AnimStart;
			}
		}

		bIsAnimationDurationLoaded = true;
	}
	return AnimationDuration;
}

UFaceFXAnimationTrack* UFaceFXAnimationSection::GetTrack() const
{
	return Cast<UFaceFXAnimationTrack>(GetOuter());
}

AActor* UFaceFXAnimationSection::GetActor() const
{
	if (UFaceFXAnimationTrack* Track = GetTrack())
	{
		UMovieScene* MovieScene = Cast<UMovieScene>(Track->GetOuter());

		FGuid TrackGuid;
		if (MovieScene && MovieScene->FindTrackBinding(*Track, TrackGuid))
		{
			if (UMovieSceneSequence* Sequence = Cast<UMovieSceneSequence>(MovieScene->GetOuter()))
			{
				if (FMovieScenePossessable* Possessable = MovieScene->FindPossessable(TrackGuid))
				{
					//fetch from possessable parent
					auto BoundObjects = Sequence->LocateBoundObjects(Possessable->GetParent(), Track);
					return BoundObjects.Num() > 0 ? Cast<AActor>(BoundObjects[0]) : nullptr;
				}
				else if (FMovieSceneSpawnable* Spawnable = MovieScene->FindSpawnable(TrackGuid))
				{
					return Cast<AActor>(Spawnable->GetObjectTemplate());
				}
			}
		}
	}
	return nullptr;
}

#if WITH_EDITOR

FText UFaceFXAnimationSection::GetTitle() const
{
	FString AnimName(LOCTEXT("SequencerTrackUnkown", "<Unknown>").ToString());

	FFaceFXAnimId AnimId = AnimationId;
	if (!AnimId.IsValid())
	{
		//fetch from asset first
		if (UFaceFXAnim* AnimAsset = Animation.Get())
		{
			AnimId = AnimAsset->GetId();
		}
	}
	if (AnimId.IsValid())
	{
		//fetch from id
		AnimName = AnimId.Group.IsNone() ? TEXT("") : AnimId.Group.ToString() + TEXT(" / ");
		AnimName += AnimId.Name.ToString();
	}
	else if (Animation.ToStringReference().IsValid())
	{
		//fetch from asset ref
		AnimName = Animation.ToStringReference().ToString();
	}

	return FText::FromString(AnimName);
}

void UFaceFXAnimationSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (AnimationId.IsValid() && Animation.IsValid())
	{
		//both ways of defining animations are valid -> perform a switch

		static const FName NameAnimation(TEXT("Animation"));
		static const FName NameAnimationId(TEXT("AnimationId"));
		if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == NameAnimation)
		{
			//Switch from animationID to asset
			AnimationId.Reset();
		}
		else if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == NameAnimationId)
		{
			//Switch from asset to animationID
			Animation.Reset();
		}
	}

	//reset status
	bIsAnimationDurationLoaded = false;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE