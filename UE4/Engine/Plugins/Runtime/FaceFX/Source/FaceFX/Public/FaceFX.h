#pragma once

//PCH includes
#include "FaceFXCharacter.h"
#include "FaceFXActor.h"
#include "FaceFXAnimSet.h"
#include "FaceFXConfig.h"

FACEFX_API DECLARE_LOG_CATEGORY_EXTERN(LogFaceFX, Display, All);

struct FACEFX_API FaceFX
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

	FaceFX() {}
};