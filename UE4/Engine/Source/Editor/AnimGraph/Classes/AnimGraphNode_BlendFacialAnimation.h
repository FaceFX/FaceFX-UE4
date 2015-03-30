// FaceFX_BEGIN
/**
* The blend node Editor UI wrapper class
* Copyright © 2014 YAGER Productions GmbH, All Rights Reserved.
*/

#pragma once

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
	ANIMGRAPH_API virtual FText GetTooltipText() const override;
	ANIMGRAPH_API virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	ANIMGRAPH_API virtual FLinearColor GetNodeTitleColor() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	ANIMGRAPH_API virtual FString GetNodeCategory() const override;
	// End of UAnimGraphNode_Base interface

	ANIMGRAPH_API virtual void CreateOutputPins() override;
};
// FaceFX_END