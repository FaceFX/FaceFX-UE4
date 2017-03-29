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

#pragma once

#include "MovieSceneEvalTemplate.h"
#include "MovieSceneExecutionToken.h"
#include "ObjectKey.h"

#include "FaceFXData.h"
#include "FaceFXAnimationSectionTemplate.generated.h"

class UFaceFXAnim;
class UFaceFXAnimationSection;
class UFaceFXAnimationTrack;


/** Data shared per sequencer track */
struct FFaceFXAnimationTrackData : IPersistentEvaluationData
{
	FFaceFXAnimationTrackData() : ActiveSectionRowIndex(INDEX_NONE) {}

	/** The active playing section */
	int32 ActiveSectionRowIndex;

	/** The FaceFX components per section rows */
	TMap<int32, FObjectKey> SectionRowFaceFXComponents;
};

/** The data per section */
struct FFaceFXAnimationSectionData
{
	/** Track that is being instanced */
	FGuid TrackId;

	/** The row index of the section in use */
	int32 RowIndex;

	/** The animation id of the section if its ID based. If its asset based, this is invalid */
	FFaceFXAnimId AnimationId;

	/** The animation asset set for a section */
	TAssetPtr<UFaceFXAnim> Animation;

	/** The component ID of that section */
	FFaceFXSkelMeshComponentId ComponentId;

	/** The total duration of the animation */
	float AnimDuration;

	/** The offset the the beginning of the animation */
	float StartOffset;

	/** The offset the the end of the animation */
	float EndOffset;

	/** The starting time of the animation */
	float StartTime;

	/** The ending time of the animation */
	float EndTime;

	FFaceFXAnimationSectionData() : RowIndex(INDEX_NONE), AnimDuration(0.F), StartOffset(0.F), EndOffset(0.F), StartTime(0.F), EndTime(0.F) {}
};

/** Execution token for Sequencer FaceFX animation section playback */
struct FFaceFXAnimationExecutionToken : public IMovieSceneExecutionToken
{
	FFaceFXAnimationExecutionToken(const FFaceFXAnimationSectionData& InSectionData = FFaceFXAnimationSectionData()) : SectionData(InSectionData) {}

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override;

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

private:

	/** The section data */
	FFaceFXAnimationSectionData SectionData;
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
	FFaceFXAnimationSectionData SectionData;
};