/**
* The blend node Editor UI wrapper class
*/

#include "FaceFxEditor.h"
#include "Animation/AnimGraphNode_BlendFacialAnimation.h"
#include "AnimationGraphSchema.h"

#define LOCTEXT_NAMESPACE "AnimGraphNodeBlendFacialAnimation"

UAnimGraphNode_BlendFacialAnimation::UAnimGraphNode_BlendFacialAnimation(const FObjectInitializer& PCIP) : Super(PCIP) 
{
}

FLinearColor UAnimGraphNode_BlendFacialAnimation::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.5f, 0.5f);
}

FText UAnimGraphNode_BlendFacialAnimation::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Blends in the bone transforms coming from the FaceFX runtime.");
}

FText UAnimGraphNode_BlendFacialAnimation::GetNodeTitle( ENodeTitleType::Type TitleType ) const 
{
	return LOCTEXT("NodeTitle", "Blend Facial Animation");
}

FString UAnimGraphNode_BlendFacialAnimation::GetNodeCategory() const
{
	return TEXT("FaceFX");
}

void UAnimGraphNode_BlendFacialAnimation::CreateOutputPins()
{
	CreatePin(EGPD_Output, GetDefault<UAnimationGraphSchema>()->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), false, false, TEXT("Pose"));
}

#undef LOCTEXT_NAMESPACE