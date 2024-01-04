/*******************************************************************************
The MIT License (MIT)
Copyright (c) 2015-2024 OC3 Entertainment, Inc. All rights reserved.
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

#include "FaceFXAudio.h"
#include "FaceFX.h"
#include "FaceFXAnim.h"

#include "FaceFXAudioImplDefault.h"
#if WITH_WWISE
#include "FaceFXAudioImplWwise.h"
#endif //WITH_WWISE

#include "HAL/IConsoleManager.h"

//Sound system to use within the FaceFX plugin.If the target sound system is invalid, the default (0, Unreal Audio System) will be used.
// Supported values :
//	0 = UnrealAudioSystem(Default)
//	1 = Wwise
#if WITH_WWISE
int32 PreferredAudioSystem = 1;
#else
int32 PreferredAudioSystem = 0;
#endif //WITH_WWISE
FAutoConsoleVariableRef CVarMaxGPUParticlesSpawnedPerFrame(TEXT("FaceFX.PreferredAudioSystem"), PreferredAudioSystem, TEXT("Sets the preferred audio system when playing audio within FaceFX. 0=UnrealAudioSystem (Default), 1=Wwise"));

AActor* IFaceFXAudio::GetOwningActor() const
{
	UFaceFXCharacter* Character = Owner.Get();
	return Character ? Character->GetOwningActor() : nullptr;
}

TSharedPtr<IFaceFXAudio> FFaceFXAudio::Create(UFaceFXCharacter* Owner)
{
#if WITH_WWISE
	if (PreferredAudioSystem == 1)
	{
		//Wwise preferred
		return MakeShareable(new FFaceFXAudioWwise(Owner));
	}
#endif //WITH_WWISE

	return MakeShareable(new FFaceFXAudioDefault(Owner));
}

bool FFaceFXAudio::IsUsingSoundWaveAssets()
{
#if WITH_WWISE
	return PreferredAudioSystem != 1;
#else
	return true;
#endif
}