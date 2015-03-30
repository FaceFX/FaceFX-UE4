#pragma once

#include "Core.h"
#include "CoreUObject.h"
#include "Engine/StreamableManager.h"

#include "FaceFxConfig.h"

//convenience includes for other modules
#include "FaceFxCharacter.h"
#include "FaceFxActor.h"
#include "FaceFxAnimSet.h"

#if WITH_EDITOR
#include "FaceFxEditorTools.h"
#endif

FACEFX_API DECLARE_LOG_CATEGORY_EXTERN(LogFaceFx, Display, All);

struct FACEFX_API FaceFx
{

	/**
	* Gets the FaceFX version
	* @returns The FaceFX version string
	*/
	static FString GetVersion();

	/**
	* Gets the FaceFX platform
	* @returns The FaceFX platform string
	*/
	static FString GetPlatform();

	/**
	* Gets the global streamer for FaceFX assets
	* @returns The streamer
	*/
	inline static FStreamableManager& GetStreamer()
	{
		return s_streamer;
	}

private:

	FaceFx() {}

	/** the global streamer for FaceFX assets */
	static FStreamableManager s_streamer;
};