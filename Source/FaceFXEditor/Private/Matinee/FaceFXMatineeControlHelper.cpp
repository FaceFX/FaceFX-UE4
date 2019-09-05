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

#include "Matinee/FaceFXMatineeControlHelper.h"
#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "Matinee/FaceFXMatineeControl.h"
#include "Animation/FaceFXComponent.h"
#include "FaceFXEditorTools.h"
#include "IMatinee.h"
#include "Matinee/MatineeActor.h"
#include "Matinee/InterpGroupInst.h"
#include "EditorModeManager.h"
#include "EditorModes.h"
#include "EditorModeInterpolation.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXMatineeControlHelper::UFaceFXMatineeControlHelper(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool UFaceFXMatineeControlHelper::PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const
{
	FEdModeInterpEdit* Mode = static_cast<FEdModeInterpEdit*>(GLevelEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_InterpEdit ));
	check(Mode);

	IMatineeBase* InterpEd = Mode->InterpEd;
	check(InterpEd);

	//check if the matinee group already contains a facefx track
	TArray<UInterpTrack*> Tracks;
	Group->FindTracksByClass(UFaceFXMatineeControl::StaticClass(), Tracks);
	if(Tracks.Num() > 0)
	{
		UE_LOG(LogFaceFX, Warning, TEXT("InterpGroup : Matinee group aready contains a FaceFX track (%s)"), *Group->GroupName.ToString());
		if(bAllowPrompts)
		{
			FFaceFXEditorTools::ShowError(LOCTEXT("MatineeFaceFXAlreadyHasTrack", "Unable to add FaceFX Track. Target matinee group already contains a FaceFX track."));
		}
		return false;
	}

	//determine the actor linked to this group. There must be ONLY one
	UInterpGroupInst* GrInst = nullptr;
	AMatineeActor* MatineeActor = InterpEd->GetMatineeActor();
	check(MatineeActor);
	for(UInterpGroupInst* GroupInst : MatineeActor->GroupInst)
	{
		if(GroupInst && GroupInst->Group == Group)
		{
			if(GrInst)
			{
				//there are more than one actor
				UE_LOG(LogFaceFX, Warning, TEXT("InterpGroup : Can't create FaceFX track for Matinee groups with more than one actor. Select a group for one actor only and try again. Group: (%s)"), *Group->GroupName.ToString());
				if(bAllowPrompts)
				{
					FFaceFXEditorTools::ShowError(LOCTEXT("MatineeFaceFXAlreadyHadGroupActor", "Can't create FaceFX track for Matinee groups with more than one actor. Select a group for one actor only and try again."));
				}
				return false;
			}
			GrInst = GroupInst;
		}
	}

	check(GrInst);

	if (AActor* Actor = GrInst->GetGroupActor())
	{
		//Locate FaceFX component
		if(!Actor->FindComponentByClass<UFaceFXComponent>())
		{
			UE_LOG(LogFaceFX, Warning, TEXT("InterpGroup : FaceFX component missing (%s)"), *Actor->GetName());
			if(bAllowPrompts)
			{
				FFaceFXEditorTools::ShowError(LOCTEXT("MatineeFaceFXMissingComponent", "Unable to add FaceFX Track. Selected actor does not own a FaceFX Component."));
			}
			return false;
		}
		return true;
	}
	else
	{
		UE_LOG(LogFaceFX, Warning, TEXT("InterpGroup : Actor missing"));
		if(bAllowPrompts)
		{
			FFaceFXEditorTools::ShowError(LOCTEXT("MatineeFaceFXMissingActor", "Unable to add FaceFX Track. No actor selected. Select an actor with a FaceFX component and try again."));
		}
	}
	return false;
}

bool UFaceFXMatineeControlHelper::PreCreateKeyframe( UInterpTrack *Track, float fTime ) const
{
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

	FEdModeInterpEdit* Mode = (FEdModeInterpEdit*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_InterpEdit);
	check(Mode);
	check(Mode->InterpEd);

	KeyDialog.ShowDialog(FaceFXComponent, FSimpleDelegate::CreateUObject(this, &UFaceFXMatineeControlHelper::OnKeyDialogClose, Mode->InterpEd, Track));

	return false;
}

void UFaceFXMatineeControlHelper::OnKeyDialogClose(IMatineeBase* Matinee, UInterpTrack* Track) const
{
	if (!KeyDialog.IsCancelled())
	{
		Matinee->FinishAddKey(Track, true);
	}
}

void UFaceFXMatineeControlHelper::PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const
{
	UFaceFXMatineeControl* TrackFaceFX = CastChecked<UFaceFXMatineeControl>(Track);
	FFaceFXTrackKey& NewAnimKey = TrackFaceFX->Keys[KeyIndex];
	NewAnimKey.Import(KeyDialog.GetSelectedAnimSet());
	NewAnimKey.bLoop = KeyDialog.IsLooping();
	KeyDialog.Reset();
}

#undef LOCTEXT_NAMESPACE
