#include "FaceFxEditor.h"
#include "FaceFxStyle.h"
#include "SlateStyle.h"
#include "ClassIconFinder.h"

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FFaceFxStyle::GetContentPath(RelativePath, ".png"), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FFaceFxStyle::StyleSet;

/**
* Gets the filepath for a file located inside the FaceFX plugin content directory
* @param RelativePath The path relative to the content directory
* @param Extension The file extension
*/
FString FFaceFxStyle::GetContentPath(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = FPaths::EnginePluginsDir() / TEXT("Runtime/FaceFX/Content");
	return (ContentDir / RelativePath) + Extension;
}

/** Initializes the style set */
void FFaceFxStyle::Initialize()
{
	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	const FVector2D Icon40(40.F, 40.F);

	StyleSet = MakeShareable(new FSlateStyleSet("FaceFxStyle"));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set("FaceFxStyle.AssetFxActor", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxactor"), Icon40));
	StyleSet->Set("FaceFxStyle.AssetFxAnimSet", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxanimset"), Icon40));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	FClassIconFinder::RegisterIconSource(StyleSet.Get());
};

#undef IMAGE_PLUGIN_BRUSH

/** Shutdown the style set */
void FFaceFxStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		FClassIconFinder::UnregisterIconSource(StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}