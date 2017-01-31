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

#include "FaceFXAnimationSectionTemplate.generated.h"

class UFaceFXAnimationSection;
class UFaceFXAnimationTrack;

/** Data shared per sequencer track */
struct FFaceFXAnimationTrackData : IPersistentEvaluationData
{
	FFaceFXAnimationTrackData() : ActiveSection(nullptr) {}

	/** The active playing section */
	const UFaceFXAnimationSection* ActiveSection;
};

/** Execution token for Sequencer FaceFX animation section playback */
struct FFaceFXAnimationExecutionToken : public IMovieSceneExecutionToken
{
	FFaceFXAnimationExecutionToken(const UFaceFXAnimationSection* Section = nullptr) : AnimationSection(Section) {}

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override;

	/** Gets the row index of the section within the track */
	int32 GetSectionRowIndex() const;

	/** Gets the id of the section's owning track */
	FGuid GetSectionTrackId() const;

private:

	/** The animation section to play */
	const UFaceFXAnimationSection* AnimationSection;
};

/** Section template for Sequencer FaceFX animation sections */
USTRUCT()
struct FFaceFXAnimationSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FFaceFXAnimationSectionTemplate(const UFaceFXAnimationSection* Section = nullptr, const UFaceFXAnimationTrack* Track = nullptr) : AnimationSection(Section), AnimationTrack(Track) {}

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;

	virtual void SetupOverrides() override
	{
		EnableOverrides(FMovieSceneEvalTemplateBase::EOverrideMask::RequiresTearDownFlag);
	}

	/** The section which is currently active within this track */
	UPROPERTY(Transient)
	const UFaceFXAnimationSection* AnimationSection;

	/** Track that is being instanced */
	UPROPERTY(Transient)
	const UFaceFXAnimationTrack* AnimationTrack;
};