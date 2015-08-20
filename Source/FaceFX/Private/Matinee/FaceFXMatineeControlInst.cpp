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
#include "Matinee/FaceFXMatineeControlInst.h"
#include "Animation/FaceFXComponent.h"

#include "Matinee/InterpGroupInst.h"
#include "Matinee/MatineeActor.h"

UFaceFXMatineeControlInst::UFaceFXMatineeControlInst(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), 
	LastUpdatePosition(0.F)
{
}

void UFaceFXMatineeControlInst::InitTrackInst(UInterpTrack* Track)
{
	UInterpGroupInst* GrInst = CastChecked<UInterpGroupInst>( GetOuter() );
	AMatineeActor* MatineeActor = CastChecked<AMatineeActor>( GrInst->GetOuter() );

	LastUpdatePosition = MatineeActor->InterpPosition;
}

void UFaceFXMatineeControlInst::RestoreActorState(UInterpTrack* Track)
{
	UInterpGroupInst* GrInst = CastChecked<UInterpGroupInst>( GetOuter() );
	
	//called when matinee closes. In that case we stop any running animation/sounds
	if(const AActor* GroupActor = GrInst->GetGroupActor())
	{
		if(UFaceFXComponent* FaceFXComp = GroupActor->FindComponentByClass<UFaceFXComponent>())
		{
			FaceFXComp->StopAll();
		}
	}
}