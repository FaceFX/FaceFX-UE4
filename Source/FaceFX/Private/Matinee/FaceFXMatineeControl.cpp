/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2018 OC3 Entertainment, Inc. All rights reserved.
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

#include "Matinee/FaceFXMatineeControl.h"
#include "FaceFX.h"
#include "Matinee/FaceFXMatineeControlInst.h"
#include "Animation/FaceFXComponent.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Matinee/InterpGroup.h"
#include "Matinee/InterpGroupInst.h"
#include "Matinee/InterpData.h"
#include "Matinee/MatineeActor.h"
#include "InterpolationHitProxy.h"
#include "CanvasTypes.h"
#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXMatineeControl::UFaceFXMatineeControl(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TrackInstClass = UFaceFXMatineeControlInst::StaticClass();
	TrackTitle = LOCTEXT("MatineeFaceFXTrack", "FaceFX Track").ToString();

#if WITH_EDITORONLY_DATA
	//TrackIcon = Cast<UTexture2D>(StaticLoadObject( UTexture2D::StaticClass(), NULL, TEXT("/Engine/EditorMaterials/MatineeGroups/MAT_Groups_Float.MAT_Groups_Float"), NULL, LOAD_None, NULL ));
#endif // WITH_EDITORONLY_DATA
}

int32 UFaceFXMatineeControl::GetNumKeyframes() const
{
	return Keys.Num();
}

float UFaceFXMatineeControl::GetTrackEndTime() const
{
	return Keys.Num() ? Keys[Keys.Num() - 1].Time : 0.0f;
}

float UFaceFXMatineeControl::GetKeyframeTime( int32 KeyIndex ) const
{
	if( KeyIndex < 0 || KeyIndex >= Keys.Num() )
	{
		return 0.f;
	}
	return Keys[KeyIndex].Time;
}

int32 UFaceFXMatineeControl::GetKeyframeIndex( float KeyTime ) const
{
	int32 RetIndex = INDEX_NONE;
	if( Keys.Num() > 0 )
	{
		float CurTime = Keys[0].Time;
		/* Loop through every keyframe until we find a keyframe with the passed in time. */
		/* Stop searching once all the keyframes left to search have larger times than the passed in time. */
		for( int32 KeyIndex = 0; KeyIndex < Keys.Num() && CurTime <= KeyTime; ++KeyIndex )
		{
			if( KeyTime == Keys[KeyIndex].Time )
			{
				RetIndex = KeyIndex;
				break;
			}
			CurTime = Keys[KeyIndex].Time;
		}
	}
	return RetIndex;
}

void UFaceFXMatineeControl::GetTimeRange( float& Time, float& EndTime ) const
{
	if(Keys.Num() == 0)
	{
		Time = 0.f;
		EndTime = 0.f;
	}
	else
	{
		Time = Keys[0].Time;
		EndTime = Keys[ Keys.Num()-1 ].Time;
	}
}

int32 UFaceFXMatineeControl::SetKeyframeTime( int32 KeyIndex, float NewKeyTime, bool bUpdateOrder)
{
	if( KeyIndex < 0 || KeyIndex >= Keys.Num() )
	{
		return KeyIndex;
	}
	if(bUpdateOrder)
	{
		/* First, remove cut from track */
		FFaceFXTrackKey MoveKey = Keys[KeyIndex];
		Keys.RemoveAt(KeyIndex);
		/* Set its time to the new one. */
		MoveKey.Time = NewKeyTime;
		/* Find correct new position and insert. */
		int32 i=0;
		for( i=0; i<Keys.Num() && Keys[i].Time < NewKeyTime; i++) {};
		Keys.InsertZeroed(i);
		Keys[i] = MoveKey;
		return i;
	}
	else
	{
		Keys[KeyIndex].Time = NewKeyTime;
		return KeyIndex;
	}
}

void UFaceFXMatineeControl::RemoveKeyframe( int32 KeyIndex )
{
	if( KeyIndex < 0 || KeyIndex >= Keys.Num() )
	{
		return;
	}
	Keys.RemoveAt(KeyIndex);
}

int32 UFaceFXMatineeControl::DuplicateKeyframe( int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack)
{
	if( KeyIndex < 0 || KeyIndex >= Keys.Num() )
	{
		return INDEX_NONE;
	}

	/* Make sure the destination track is specified. */
	UFaceFXMatineeControl* DestTrack = this;
	if ( ToTrack )
	{
		DestTrack = CastChecked< UFaceFXMatineeControl >( ToTrack );
	}
	FFaceFXTrackKey NewKey = Keys[KeyIndex];
	NewKey.Time = NewKeyTime;
	/* Find the correct index to insert this key. */
	int32 i=0; for( i=0; i<DestTrack->Keys.Num() && DestTrack->Keys[i].Time < NewKeyTime; i++) {};
	DestTrack->Keys.InsertZeroed(i);
	DestTrack->Keys[i] = NewKey;
	return i;
}

bool UFaceFXMatineeControl::GetClosestSnapPosition( float InPosition, TArray<int32>& IgnoreKeys, float& OutPosition )
{
	if(Keys.Num() == 0)
	{
		return false;
	}
	bool bFoundSnap = false;
	float ClosestSnap = 0.f;
	float ClosestDist = BIG_NUMBER;
	for(int32 i=0; i<Keys.Num(); i++)
	{
		if(!IgnoreKeys.Contains(i))
		{
			float Dist = FMath::Abs( Keys[i].Time - InPosition );
			if(Dist < ClosestDist)
			{
				ClosestSnap = Keys[i].Time;
				ClosestDist = Dist;
				bFoundSnap = true;
			}
		}
	}
	OutPosition = ClosestSnap;
	return bFoundSnap;
}

int32 UFaceFXMatineeControl::AddKeyframe( float Time, UInterpTrackInst* TrackInst, EInterpCurveMode InitInterpMode )
{
	UFaceFXMatineeControlInst* TrackInstFaceFX = CastChecked<UFaceFXMatineeControlInst>(TrackInst);

	FFaceFXTrackKey NewKey;
	NewKey.Time = Time;

	//insert and order by time
	int32 i = 0;
	for(i = 0; i < Keys.Num() && Keys[i].Time < Time; ++i);
	Keys.Insert(NewKey, i);

	UpdateKeyframe(i, TrackInst);
	return i;
}


void UFaceFXMatineeControl::PreviewUpdateTrack( float NewPosition, UInterpTrackInst* TrackInst )
{
	UInterpGroupInst* GrInst = CastChecked<UInterpGroupInst>( TrackInst->GetOuter() );
	AMatineeActor* MatineeActor = CastChecked<AMatineeActor>( GrInst->GetOuter() );

	const bool bJump = !(MatineeActor->bIsPlaying);

	UpdateTrack(NewPosition, TrackInst, bJump);

	if(AActor* Actor = TrackInst->GetGroupActor())
	{
		TInlineComponentArray<USkeletalMeshComponent*> SkelMeshComps;
		Actor->GetComponents(SkelMeshComps);

		const UFaceFXMatineeControlInst* TrackFaceFX = CastChecked<UFaceFXMatineeControlInst>(TrackInst);
		const float TimeElapsed = (bJump || NewPosition < TrackFaceFX->LastUpdatePosition) ? 0.F : NewPosition - TrackFaceFX->LastUpdatePosition;

		for(USkeletalMeshComponent* SkelMeshComp : SkelMeshComps)
		{
			SkelMeshComp->RefreshBoneTransforms();
			SkelMeshComp->RefreshSlaveComponents();
			SkelMeshComp->UpdateComponentToWorld();
		}
	}
}

void UFaceFXMatineeControl::PreviewStopPlayback(UInterpTrackInst* TrInst)
{
	check(TrInst);
	AActor* Actor = TrInst->GetGroupActor();
	if(!Actor)
	{
		return;
	}

	if(UFaceFXComponent* FaceFXComp = Actor->FindComponentByClass<UFaceFXComponent>())
	{
		//playback stopped
		FaceFXComp->StopAll();
	}
}

void UFaceFXMatineeControl::GetTrackKeyForTime(float InTime, TArray<TPair<int32, const FFaceFXTrackKey*>>& OutResult, TArray<FFaceFXSkelMeshComponentId>* OutNoTracks) const
{
	//build a list of all keys for all skelmesh component ids
	TMap<int32, TArray<const FFaceFXTrackKey*>> SkelMeshTracks;
	TMap<int32, FFaceFXSkelMeshComponentId> SkelMeshIds;
	for(const FFaceFXTrackKey& Key : Keys)
	{
		SkelMeshTracks.FindOrAdd(Key.SkelMeshComponentId.Index).Add(&Key);
		if(OutNoTracks && !SkelMeshIds.Contains(Key.SkelMeshComponentId.Index))
		{
			SkelMeshIds.Add(Key.SkelMeshComponentId.Index, Key.SkelMeshComponentId);
		}
	}

	//then generate the pair results for each skelmesh component
	for(auto It = SkelMeshTracks.CreateConstIterator(); It; ++It)
	{
		const TArray<const FFaceFXTrackKey*>& SkelMeshKeys = It.Value();

		const int32 IndexMax = SkelMeshKeys.Num()-1;
		int32 Index = INDEX_NONE;
		for(; Index < IndexMax && SkelMeshKeys[Index+1]->Time <= InTime; ++Index);

		if(Index != INDEX_NONE)
		{
			OutResult.Add(TPairInitializer<int32, const FFaceFXTrackKey*>(Index, SkelMeshKeys[Index]));
		}
		else if(OutNoTracks)
		{
			OutNoTracks->Add(SkelMeshIds.FindChecked(It.Key()));
		}
	}
}

/**
* Loads the animation from a given track keyframe
* @param Track The keyframe to load the animation from
* @param Owner The owner of the potentially loaded animation data
* @returns The loaded animation, else nullptr if not found or unset
*/
UFaceFXAnim* GetAnimation(const FFaceFXTrackKey& Track, UObject* Owner)
{
	UFaceFXAnim* NewAnim = Track.Animation.Get();
	if(!NewAnim && Track.Animation.ToSoftObjectPath().IsValid())
	{
		NewAnim = Cast<UFaceFXAnim>(StaticLoadObject(UFaceFXAnim::StaticClass(), Owner, *Track.Animation.ToSoftObjectPath().ToString()));
	}

	return NewAnim;
}

void UFaceFXMatineeControl::UpdateTrack( float NewPosition, UInterpTrackInst* TrackInst, bool bJump )
{
	check(TrackInst);

	AActor* Actor = TrackInst->GetGroupActor();
	if(!Actor)
	{
		return;
	}

	UFaceFXComponent* FaceFXComp = Actor->FindComponentByClass<UFaceFXComponent>();
	if(!FaceFXComp)
	{
		return;
	}

	UFaceFXMatineeControlInst* TrackInstFaceFX = CastChecked<UFaceFXMatineeControlInst>(TrackInst);

	if(Keys.Num() == 0)
	{
		FaceFXComp->Stop(nullptr, this);
	}
	else if(NewPosition <= TrackInstFaceFX->LastUpdatePosition || bJump)
	{
		TArray<TPair<int32, const FFaceFXTrackKey*>> TrackKeyPairs;
		TArray<FFaceFXSkelMeshComponentId> SkelMeshNoTracks;
		GetTrackKeyForTime(NewPosition, TrackKeyPairs, &SkelMeshNoTracks);

		for(const TPair<int32, const FFaceFXTrackKey*>& TrackKeyPair : TrackKeyPairs)
		{
			const FFaceFXTrackKey* TrackKey = TrackKeyPair.Value;

			const float NewFaceFXAnimLocation = NewPosition - TrackKey->Time;
			check(NewFaceFXAnimLocation >= 0.F)

			USkeletalMeshComponent* SkelMeshTarget = FaceFXComp->GetSkelMeshTarget(TrackKey->SkelMeshComponentId);

			//Always stop when we switch track keys to prevent playing animations when switching backward after the end of another track animation
			//Thats because we don't check here how long the animation actually plays and rely on the JumpTo/Play functionality alone to determine that
			if(TrackKeyPair.Key != TrackInstFaceFX->GetCurrentTrackIndex(SkelMeshTarget))
			{
				FaceFXComp->Stop(SkelMeshTarget);
			}
			TrackInstFaceFX->SetCurrentTrackIndex(SkelMeshTarget, TrackKeyPair.Key);

			bool JumpSucceeded = false;

			//playing backwards or jumping
			if(TrackKey->AnimationId.IsValid())
			{
				//play by animation id
				JumpSucceeded = FaceFXComp->JumpToById(NewFaceFXAnimLocation, bJump, TrackKey->AnimationId.Group, TrackKey->AnimationId.Name, TrackKey->bLoop, SkelMeshTarget, this);
			}
			else if(UFaceFXAnim* FaceFXAnim = GetAnimation(*TrackKey, this))
			{
				//play by animation
				JumpSucceeded = FaceFXComp->JumpTo(NewFaceFXAnimLocation, bJump, FaceFXAnim, TrackKey->bLoop, SkelMeshTarget, this);
			}

			if(!JumpSucceeded)
			{
				//jump to failed -> i.e. out of range on non looping animation
				FaceFXComp->Stop(SkelMeshTarget);
			}
		}

		for(const FFaceFXSkelMeshComponentId& SkelMashNoTrack : SkelMeshNoTracks)
		{
			//no track key at this position for this skelmesh component -> stop
			FaceFXComp->Stop(FaceFXComp->GetSkelMeshTarget(SkelMashNoTrack));
		}
	}
	else
	{
		TArray<TPair<int32, const FFaceFXTrackKey*>> TrackKeyPairs;
		GetTrackKeyForTime(NewPosition, TrackKeyPairs);

		for(const TPair<int32, const FFaceFXTrackKey*>& TrackKeyPair : TrackKeyPairs)
		{
			const FFaceFXTrackKey* TrackKey = TrackKeyPair.Value;

			USkeletalMeshComponent* SkelMeshTarget = FaceFXComp->GetSkelMeshTarget(TrackKey->SkelMeshComponentId);

			const bool IsPaused = FaceFXComp->IsPaused(SkelMeshTarget, this);
			const bool IsPlayingOrPaused = IsPaused || FaceFXComp->IsPlaying(SkelMeshTarget, this);

			if(!IsPlayingOrPaused || TrackKeyPair.Key != TrackInstFaceFX->GetCurrentTrackIndex(SkelMeshTarget))
			{
				//start playback of a new animation

				const float PlaybackStartPosition = NewPosition - TrackKey->Time;
				checkf(PlaybackStartPosition >= 0.F, TEXT("Invalid animation start location"));

				bool StartSucceeded = false;

				//play by animation id
				if(TrackKey->AnimationId.IsValid())
				{
					//play by animation id
					StartSucceeded = FaceFXComp->JumpToById(PlaybackStartPosition, false, TrackKey->AnimationId.Group, TrackKey->AnimationId.Name, TrackKey->bLoop, SkelMeshTarget, this);
				}
				else if(UFaceFXAnim* FaceFXAnim = GetAnimation(*TrackKey, this))
				{
					//play by animation
					StartSucceeded = FaceFXComp->JumpTo(PlaybackStartPosition, false, FaceFXAnim, TrackKey->bLoop, SkelMeshTarget, this);
				}

				if(!StartSucceeded)
				{
					//start failed -> stop
					FaceFXComp->Stop(SkelMeshTarget);
				}

				TrackInstFaceFX->SetCurrentTrackIndex(SkelMeshTarget, TrackKeyPair.Key);
			}
			else if(IsPaused)
			{
				//resume paused animation
				FaceFXComp->Resume(SkelMeshTarget, this);
			}
		}
	}

	TrackInstFaceFX->LastUpdatePosition = NewPosition;
}

void UFaceFXMatineeControl::DrawTrack( FCanvas* Canvas, UInterpGroup* Group, const FInterpTrackDrawParams& Params )
{
	static const FColor	KeySelectedColor(255,128,0);
	static const FColor KeyLabelColor(225,225,225);
	static const FColor KeyBackgroundColor(0,150,200);
	static const FColor KeyColorBlack(0,0,0);
	static const int32	KeyVertOffset = 3;

	const bool bHitTesting = Canvas->IsHitTesting();
	const bool bAllowBarSelection = bHitTesting && Params.bAllowKeyframeBarSelection;
	const bool bAllowTextSelection = bHitTesting && Params.bAllowKeyframeTextSelection;

	AMatineeActor* MatineeActor = CastChecked<AMatineeActor>(Group->GetOuter()->GetOuter());
	UInterpGroupInst* GroupInst = MatineeActor->FindFirstGroupInst(Group);
	const AActor* GroupActor = GroupInst ? GroupInst->GetGroupActor() : nullptr;

	//cached animation ids
	TArray<FFaceFXAnimId> AnimIds;
	AnimIds.AddUninitialized(Keys.Num());

	//Draw the tiles for each animation
	for (int32 i = 0; i < Keys.Num(); i++)
	{
		const FFaceFXTrackKey& AnimKey = Keys[i];

		FFaceFXAnimId AnimationId;
		if(AnimKey.AnimationId.IsValid())
		{
			AnimationId = AnimKey.AnimationId;
		}
		else if(const UFaceFXAnim* Animation = GetAnimation(AnimKey, this))
		{
			AnimationId = Animation->GetId();
		}

		AnimIds[i] = AnimationId;

		//determine animation duration
		const float AnimStartTime = AnimKey.Time;
		const float AnimEndTime = AnimStartTime + AnimKey.GetAnimationDuration(GroupActor);

		const int32 StartPixelPos = FMath::TruncToInt((AnimStartTime - Params.StartTime) * Params.PixelsPerSec);
		const int32 EndPixelPos = FMath::TruncToInt((AnimEndTime - Params.StartTime) * Params.PixelsPerSec);

		// Find if this sound is one of the selected ones.
		bool bKeySelected = false;
		for (const FInterpEdSelKey& SelectedKey : Params.SelectedKeys)
		{
			if(SelectedKey.Group == Group && SelectedKey.Track == this && SelectedKey.KeyIndex == i)
			{
				bKeySelected = true;
				break;
			}
		}

		const FColor& BorderColor = bKeySelected ? KeySelectedColor : KeyColorBlack;

		if(bAllowBarSelection)
		{
			Canvas->SetHitProxy(new HInterpTrackKeypointProxy(Group, this, i));
		}
		Canvas->DrawTile(StartPixelPos, KeyVertOffset, EndPixelPos - StartPixelPos + 1, FMath::TruncToFloat(Params.TrackHeight - 2.f*KeyVertOffset), 0.f, 0.f, 1.f, 1.f, BorderColor);
		Canvas->DrawTile(StartPixelPos+1, KeyVertOffset+1, EndPixelPos - StartPixelPos - 1, FMath::TruncToFloat(Params.TrackHeight - 2.f*KeyVertOffset) - 2, 0.f, 0.f, 1.f, 1.f, KeyBackgroundColor);
		if(bAllowBarSelection)
		{
			Canvas->SetHitProxy(nullptr);
		}
	}

	// Use base-class to draw key triangles
	Super::DrawTrack( Canvas, Group, Params );

	//Draw animation names on top
	for (int32 i = 0; i < Keys.Num(); i++)
	{
		const FFaceFXTrackKey& AnimKey = Keys[i];

		//fetch the cached animation id for the key
		const FFaceFXAnimId& AnimId = AnimIds[i];

		FString AnimName(LOCTEXT("MatineeFaceFXTrackUnkown", "<Unknown>").ToString());
		if(AnimId.IsValid())
		{
			AnimName = AnimId.Group.IsNone() ? TEXT("") : AnimId.Group.ToString() + TEXT(" / ");
			AnimName += AnimId.Name.ToString();
		}

		if(AnimKey.bLoop)
		{
			AnimName += TEXT(" [") + LOCTEXT("MatineeFaceFXTrackLooping", "Looping").ToString() + TEXT("]");
		}

		int32 XL, YL;
		StringSize(GEngine->GetSmallFont(), XL, YL, *AnimName);

		if (bAllowTextSelection)
		{
			Canvas->SetHitProxy(new HInterpTrackKeypointProxy(Group, this, i));
		}

		const int32 PixelPos = FMath::TruncToInt((AnimKey.Time - Params.StartTime) * Params.PixelsPerSec);

		Canvas->DrawShadowedString((PixelPos + 2), Params.TrackHeight - YL - KeyVertOffset, *AnimName, GEngine->GetSmallFont(), KeyLabelColor);
		if (bAllowTextSelection)
		{
			Canvas->SetHitProxy(nullptr);
		}
	}
}

float FFaceFXTrackKey::GetAnimationDuration(const AActor* Actor) const
{
	if(!bIsAnimationDurationLoaded)
	{
#if FACEFX_USEANIMATIONLINKAGE
		//try to resolve the animation duration
		if(AnimationId.IsValid() && Actor)
		{
			//resolve by animation id
			float AnimStart, AnimEnd;
			if(UFaceFXCharacter::GetAnimationBoundsById(Actor, AnimationId, AnimStart, AnimEnd))
			{
				AnimationDuration = AnimEnd - AnimStart;
			}
		}
		else
#endif //FACEFX_USEANIMATIONLINKAGE
		if(UFaceFXAnim* TargetAnimation = GetAnimation(*this, nullptr))
		{
			//resolve by animation asset
			float AnimStart, AnimEnd;
			if(FaceFX::GetAnimationBounds(TargetAnimation, AnimStart, AnimEnd))
			{
				AnimationDuration = AnimEnd - AnimStart;
			}
		}

		bIsAnimationDurationLoaded = true;
	}
	return AnimationDuration;
}

#undef LOCTEXT_NAMESPACE
