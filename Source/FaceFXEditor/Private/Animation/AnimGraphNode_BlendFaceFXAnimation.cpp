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

#include "Animation/AnimGraphNode_BlendFaceFXAnimation.h"
#include "FaceFXEditor.h"
#include "AnimationGraphSchema.h"

#define LOCTEXT_NAMESPACE "FaceFX"

UAnimGraphNode_BlendFaceFXAnimation::UAnimGraphNode_BlendFaceFXAnimation(const FObjectInitializer& PCIP) : Super(PCIP)
{
}

FLinearColor UAnimGraphNode_BlendFaceFXAnimation::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.5f, 0.5f);
}

FText UAnimGraphNode_BlendFaceFXAnimation::GetTooltipText() const
{
	return LOCTEXT("BlendAnimationNodeTooltip", "Blends in the bone transforms coming from the FaceFX runtime.");
}

FText UAnimGraphNode_BlendFaceFXAnimation::GetNodeTitle( ENodeTitleType::Type TitleType ) const
{
	return LOCTEXT("BlendAnimationNodeTitle", "Blend FaceFX Animation");
}

FString UAnimGraphNode_BlendFaceFXAnimation::GetNodeCategory() const
{
	return TEXT("FaceFX");
}

void UAnimGraphNode_BlendFaceFXAnimation::CreateOutputPins()
{
	CreatePin(EGPD_Output, GetDefault<UAnimationGraphSchema>()->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}

#undef LOCTEXT_NAMESPACE
