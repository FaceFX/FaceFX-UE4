/**
* The blend node Editor UI wrapper class
*/

#pragma once

#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_BlendFacialAnimation.h"
#include "AnimGraphNode_BlendFacialAnimation.generated.h"

UCLASS()
class UAnimGraphNode_BlendFacialAnimation : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/** The associated node */
    UPROPERTY(EditAnywhere, Category=Settings)
    FAnimNode_BlendFacialAnimation Node;

	// UEdGraphNode interface
	FACEFXEDITOR_API virtual FText GetTooltipText() const override;
	FACEFXEDITOR_API virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FACEFXEDITOR_API virtual FLinearColor GetNodeTitleColor() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	FACEFXEDITOR_API virtual FString GetNodeCategory() const override;
	// End of UAnimGraphNode_Base interface

	FACEFXEDITOR_API virtual void CreateOutputPins() override;
};