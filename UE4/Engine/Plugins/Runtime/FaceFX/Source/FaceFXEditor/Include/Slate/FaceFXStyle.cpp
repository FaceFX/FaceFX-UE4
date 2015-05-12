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

#include "FaceFXEditor.h"
#include "FaceFXStyle.h"
#include "SlateStyle.h"
#include "ClassIconFinder.h"

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FFaceFXStyle::GetContentPath(RelativePath, ".png"), __VA_ARGS__ )

static FName s_BrushIdActor(TEXT("FaceFXStyle.AssetFXActor"));
static FName s_BrushIdAnim(TEXT("FaceFXStyle.AssetFXAnim"));
static FName s_BrushIdSuccess(TEXT("FaceFXStyle.IconSuccess"));
static FName s_BrushIdWarn(TEXT("FaceFXStyle.IconWarn"));
static FName s_BrushIdError(TEXT("FaceFXStyle.IconError"));

TSharedPtr<FSlateStyleSet> FFaceFXStyle::StyleSet;

/**
* Gets the filepath for a file located inside the FaceFX plugin content directory
* @param RelativePath The path relative to the content directory
* @param Extension The file extension
*/
FString FFaceFXStyle::GetContentPath(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = FPaths::EnginePluginsDir() / TEXT("Runtime/FaceFX/Content");
	return (ContentDir / RelativePath) + Extension;
}

/** Initializes the style set */
void FFaceFXStyle::Initialize()
{
	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	const FVector2D Icon40(40.F, 40.F);

	StyleSet = MakeShareable(new FSlateStyleSet("FaceFXStyle"));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set(s_BrushIdActor, new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxactor"), Icon40));
	StyleSet->Set(s_BrushIdAnim, new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxanim"), Icon40));
	
	const FVector2D Icon16(16.F, 16.F);
	StyleSet->Set(s_BrushIdSuccess, new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxsuccess"), Icon16));
	StyleSet->Set(s_BrushIdWarn, new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxwarning"), Icon16));
	StyleSet->Set(s_BrushIdError, new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxerror"), Icon16));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	FClassIconFinder::RegisterIconSource(StyleSet.Get());
};

#undef IMAGE_PLUGIN_BRUSH

/** Shutdown the style set */
void FFaceFXStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		FClassIconFinder::UnregisterIconSource(StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

const FName& FFaceFXStyle::GetBrushIdFxActor()
{
	return s_BrushIdActor;
}

const FName& FFaceFXStyle::GetBrushIdFxAnim()
{
	return s_BrushIdAnim;
}

const FSlateBrush* FFaceFXStyle::GetBrushStateIconSuccess()
{
	return StyleSet->GetBrush(s_BrushIdSuccess);
}

const FSlateBrush* FFaceFXStyle::GetBrushStateIconWarning()
{
	return StyleSet->GetBrush(s_BrushIdWarn);
}

const FSlateBrush* FFaceFXStyle::GetBrushStateIconError()
{
	return StyleSet->GetBrush(s_BrushIdError);
}