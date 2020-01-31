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

#pragma once

#include "CoreMinimal.h"

struct FSlateBrush;

/** A wrapper for the FaceFX related slate style sets */
struct FFaceFXStyle
{
	/** Initializes the style set */
	static void Initialize();

	/** Shutdown the style set */
	static void Shutdown();

	/**
	* Gets the brush id for the actor brush
	* @returns The id
	*/
	static const FName& GetBrushIdFxActor();

	/**
	* Gets the brush id for the animation brush
	* @returns The id
	*/
	static const FName& GetBrushIdFxAnim();

	/**
	* Gets the brush for the success icon
	* @returns The slate brush
	*/
	static const FSlateBrush* GetBrushStateIconSuccess();

	/**
	* Gets the brush for the warning icon
	* @returns The slate brush
	*/
	static const FSlateBrush* GetBrushStateIconWarning();

	/**
	* Gets the brush for the error icon
	* @returns The slate brush
	*/
	static const FSlateBrush* GetBrushStateIconError();

private:

    /**
    * Gets the filepath for a file located inside the FaceFX plugin resources directory
    * @param RelativePath The path relative to the resources directory
    * @param Extension The file extension
    */
    static FString GetResourcePath(const FString& RelativePath, const ANSICHAR* Extension);

	/** The style set instance */
	static TSharedPtr< class FSlateStyleSet > StyleSet;
};
