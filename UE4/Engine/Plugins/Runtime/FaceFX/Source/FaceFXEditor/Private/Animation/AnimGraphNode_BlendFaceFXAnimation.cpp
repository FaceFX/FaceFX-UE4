/** The blend node Editor UI wrapper class */

#include "FaceFXEditor.h"
#include "Animation/AnimGraphNode_BlendFaceFXAnimation.h"
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
	CreatePin(EGPD_Output, GetDefault<UAnimationGraphSchema>()->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), false, false, TEXT("Pose"));
}

#undef LOCTEXT_NAMESPACE