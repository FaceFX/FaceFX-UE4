#pragma once

/** A wrapper for the FaceFX related slate style sets */
struct FFaceFxStyle
{
	/** Initializes the style set */
	static void Initialize();

	/** Shutdown the style set */
	static void Shutdown();

private:
	
	/**
	* Gets the filepath for a file located inside the FaceFX plugin content directory
	* @param RelativePath The path relative to the content directory
	* @param Extension The file extension
	*/
	static FString GetContentPath(const FString& RelativePath, const ANSICHAR* Extension);
	
	/** The style set instance */
	static TSharedPtr< class FSlateStyleSet > StyleSet;
};