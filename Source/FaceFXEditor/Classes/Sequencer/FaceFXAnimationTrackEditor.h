/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2019 OC3 Entertainment, Inc. All rights reserved.
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

#include "MovieSceneTrackEditor.h"
#include "Include/Slate/FaceFXAnimationKeyDetailsDialog.h"

class UFaceFXAnim;

/** Track editor for sequencer support */
class FFaceFXAnimationTrackEditor : public FMovieSceneTrackEditor
{
public:

	/**
	* Constructor
	* @param InSequencer The sequencer instance to be used by track this editor instance
	*/
	FFaceFXAnimationTrackEditor(TSharedRef<ISequencer> InSequencer) : FMovieSceneTrackEditor(InSequencer), KeyDialog(false) {}
	virtual ~FFaceFXAnimationTrackEditor() { }

	/**
	* Creates an instance of this class.  Called by a sequencer
	* @param OwningSequencer The sequencer instance to be used by this tool
	* @return The new instance of this class
	*/
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

	/**
	* Gets the currently opened sequencer instance
	* @returns The sequencer instance or null weak ptr
	*/
	static inline const TWeakPtr<ISequencer>& GetCurrentSequencer()
	{
		return CurrentSequencer;
	}

	//ISequencerTrackEditor
	virtual void AddKey(const FGuid& ObjectGuid) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	//~ISequencerTrackEditor

private:

	inline void OnAddKey(FGuid ObjectGuid)
	{
		AddKey(ObjectGuid);
	}

	/** Creates the add outliner widget */
	TSharedRef<SWidget> CreateOutlinerWidget(FGuid ObjectBinding);

	/** Adds a new FaceFX section */
	FKeyPropertyResult AddFaceFXSection(FFrameNumber KeyTime, UObject* Object, FFaceFXAnimComponentSet AnimCompSet);

	/** Callback for when the add FaceFX track dialog was closed */
	void OnFaceFXTrackDialogClosed(FGuid ObjectBinding);

	/** Gets the FaceFX component for the given GUID */
	class UFaceFXComponent* GetFaceFXComponent(const FGuid& Guid);

	/** The key creation dialog */
	FFaceFXAnimationKeyDetailsDialog KeyDialog;

	/** The currently open sequencer */
	static TWeakPtr<ISequencer> CurrentSequencer;
};

/** FaceFX animation sections */
class FFaceFXAnimationSection : public ISequencerSection, public TSharedFromThis < FFaceFXAnimationSection >
{
public:

	FFaceFXAnimationSection(UMovieSceneSection& InSection) : Section(InSection){}
	virtual ~FFaceFXAnimationSection() { }

	//ISequencerSection
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;
	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
	//~ISequencerSection

private:

	/** The section we are visualizing */
	UMovieSceneSection& Section;
};
