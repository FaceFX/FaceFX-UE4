#pragma once

//PCH includes
#include "FaceFxCharacter.h"
#include "FaceFxActor.h"
#include "FaceFxAnimSet.h"
#include "FaceFxConfig.h"

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
	static struct FStreamableManager& GetStreamer();

private:

	FaceFx() {}
};