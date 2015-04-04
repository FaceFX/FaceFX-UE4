#include "FaceFXEditor.h"
#include "FaceFXStyle.h"
#include "SlateStyle.h"
#include "ClassIconFinder.h"

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FFaceFXStyle::GetContentPath(RelativePath, ".png"), __VA_ARGS__ )

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

	StyleSet->Set("FaceFXStyle.AssetFXActor", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxactor"), Icon40));
	StyleSet->Set("FaceFXStyle.AssetFXAnimSet", new IMAGE_PLUGIN_BRUSH(TEXT("Icons/facefxanimset"), Icon40));

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