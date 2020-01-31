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

#include "Evaluation/MovieSceneEvalTemplate.h"
#include "MovieSceneExecutionToken.h"
#include "UObject/ObjectKey.h"
#include "Misc/FrameNumber.h"

#include "FaceFXData.h"
#include "FaceFXAnimationSectionTemplate.generated.h"

class UFaceFXAnim;
class UFaceFXAnimationSection;
class UFaceFXAnimationTrack;


/** Data shared per sequencer track */
struct FFaceFXAnimationTrackData
{
	FFaceFXAnimationTrackData() : ActiveSectionRowIndex(INDEX_NONE) {}

	/** The active playing section */
	int32 ActiveSectionRowIndex;

	/** The FaceFX components per section rows */
	TMap<int32, FObjectKey> SectionRowFaceFXComponents;

	inline bool IsValid() const
	{
		return ActiveSectionRowIndex != INDEX_NONE;
	}
};

/** The shared data container containing all FaceFX track data */
struct FFaceFXAnimationSharedData : IPersistentEvaluationData
{
	/** The per track data */
	TMap<FGuid, FFaceFXAnimationTrackData> TrackData;
};

/** The data per section */
USTRUCT()
struct FFaceFXAnimationSectionData
{
	GENERATED_BODY()

	/** Track that is being instanced */
	UPROPERTY()
	FGuid TrackId;

	/** The row index of the section in use */
	UPROPERTY()
	int32 RowIndex;

	/** The animation id of the section if its ID based. If its asset based, this is invalid */
	UPROPERTY()
	FFaceFXAnimId AnimationId;

	/** The animation asset set for a section */
	UPROPERTY()
	TSoftObjectPtr<UFaceFXAnim> Animation;

	/** The component ID of that section */
	UPROPERTY()
	FFaceFXSkelMeshComponentId ComponentId;

	/** The total duration of the animation */
	UPROPERTY()
	float AnimDuration;

	/** The offset the the beginning of the animation */
	UPROPERTY()
	float StartOffset;

	/** The offset the the end of the animation */
	UPROPERTY()
	float EndOffset;

	/** The starting time of the animation */
	UPROPERTY()
	FFrameNumber StartTime;

	/** The ending time of the animation */
	UPROPERTY()
	FFrameNumber EndTime;

	inline bool IsValid() const
	{
		return TrackId.IsValid() && RowIndex != INDEX_NONE;
	}

	FFaceFXAnimationSectionData() : RowIndex(INDEX_NONE), AnimDuration(0.F), StartOffset(0.F), EndOffset(0.F) {}
};

/** Execution token for Sequencer FaceFX animation section playback */
struct FFaceFXAnimationExecutionToken : public IMovieSceneSharedExecutionToken
{
	FFaceFXAnimationExecutionToken(const FFaceFXAnimationSectionData& InSectionData = FFaceFXAnimationSectionData(), const FMovieSceneEvaluationOperand& InOperand = FMovieSceneEvaluationOperand(), 
		const FMovieSceneContext& InContext = FMovieSceneContext(FMovieSceneEvaluationRange(0, FFrameRate()))) : SectionData(InSectionData), Operand(InOperand), Context(InContext) {}

	virtual void Execute(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override;

	/** Gets the row index of the section within the track */
	inline int32 GetSectionRowIndex() const
	{
		return SectionData.RowIndex;
	}

	/** Gets the id of the section's owning track */
	inline const FGuid& GetSectionTrackId() const
	{
		return SectionData.TrackId;
	}

	/** Gets the section data */
	inline const FFaceFXAnimationSectionData& GetSectionData() const
	{
		return SectionData;
	}

	/** Gets the assigned operand */
	inline const FMovieSceneEvaluationOperand& GetOperand() const
	{
		return Operand;
	}

private:

	/** The section data */
	FFaceFXAnimationSectionData SectionData;

	/** The evaluation operand */
	FMovieSceneEvaluationOperand Operand;

	/** The evaluation context */
	FMovieSceneContext Context;
};

/** Section template for Sequencer FaceFX animation sections */
USTRUCT()
struct FFaceFXAnimationSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FFaceFXAnimationSectionTemplate(const UFaceFXAnimationSection* Section = nullptr, const UFaceFXAnimationTrack* Track = nullptr);

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;

	virtual void SetupOverrides() override
	{
		EnableOverrides(FMovieSceneEvalTemplateBase::EOverrideMask::RequiresTearDownFlag);
	}

	/** The section data */
	UPROPERTY()
	FFaceFXAnimationSectionData SectionData;
};
