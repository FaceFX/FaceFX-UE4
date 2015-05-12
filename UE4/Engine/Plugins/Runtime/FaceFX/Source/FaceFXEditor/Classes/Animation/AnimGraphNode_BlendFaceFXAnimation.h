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

#pragma once

#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_BlendFaceFXAnimation.h"
#include "AnimGraphNode_BlendFaceFXAnimation.generated.h"

/** The blend node Editor UI wrapper class */
UCLASS()
class UAnimGraphNode_BlendFaceFXAnimation : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/** The associated node */
    UPROPERTY(EditAnywhere, Category=Settings)
    FAnimNode_BlendFaceFXAnimation Node;

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