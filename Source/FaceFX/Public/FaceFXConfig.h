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
#include "Runtime/Launch/Resources/Version.h"
#include "FaceFXLib/facefx-runtime-1.5.1/facefx/facefx.h"

#include "FaceFXConfig.generated.h"

// The number of total FaceFX channels. One channel per animation we want to be
// able to play at once per FaceFXCharacter. Default Value: 1
#define FACEFX_CHANNELS 1

// Indicator if the FaceFX Actor assets shall maintain links to any imported
// FaceFX Animation asset. Default Value: 1
// When true those linked assets will always be cooked and playback can be
// triggered using ID Strings (and assets). This imply some additional memory
// overhead and cooking of possibly unused animations
// When false playback can only be triggered using an asset reference.
// Maintaining such assets for cooking is the responsibility of the developer
#define FACEFX_USEANIMATIONLINKAGE 1

// Indicator if imported .ffxanim will be deleted. Default Value: 1
// When true those asset will be deleted after import and considered as
// up-to-date when the asset source file does exist during reimport.
// When false those files are not deleted and are considered always newer than
// asset. Reimport will always be performed
#define FACEFX_DELETE_IMPORTED_ANIM 1

// Indicator if the whole .ffxc folder shall be deleted when there is no
// .ffxanim left inside after loading compiled data. Default Value: 1
#define FACEFX_DELETE_EMPTY_COMPILATION_FOLDER 1

// The root namespace for any ini file entry
#define FACEFX_CONFIG_NS TEXT("FaceFX")

// File extension: FaceFX Studio asset
#define FACEFX_FILEEXT_FACEFX TEXT(".facefx")

// File extension: Animation data
#define FACEFX_FILEEXT_ANIM TEXT(".ffxanim")

// File extension search filter: Animation data
#define FACEFX_FILEEXT_ANIMSEARCH TEXT("*.ffxanim")

// File extension: Bone map
#define FACEFX_FILEEXT_BONES TEXT(".ffxbones")

// File extension: FaceFX Actor
#define FACEFX_FILEEXT_ACTOR TEXT(".ffxactor")

// File extension: Animation Ids
#define FACEFX_FILEEXT_ANIMID TEXT(".ffxids")

// File extension: Audio map
#define FACEFX_FILEEXT_AUDIO TEXT(".ffxamap")

// File dialog filter for FaceFX Actor source assets
#define FACEFX_FILEFILTER_ASSET_ACTOR TEXT("FaceFX Asset (*.facefx)|*.facefx;")

// File dialog filter for FaceFX Animation source assets
#define FACEFX_FILEFILTER_ASSET_ANIM                                           \
  TEXT("FaceFX Animation Asset (*.ffxanim)|*.ffxanim;")

// Support for sequencer which was added with UE 4.12.
#if ENGINE_MAJOR_VERSION < 4 && ENGINE_MINOR_VERSION < 13
#error                                                                         \
    "FaceFX Sequencer support requires Unreal Engine 4.12 or higher. Please update your engine or use a previous version of the plugin.";
#endif


/** Blend mode for FaceFX runtime */
UENUM()
enum class EFaceFXBlendMode : uint8
{
    /** Overwrite global settings with replace mode. The modifier replaces the existing translation, rotation, or scale. */
    Replace UMETA(DisplayName = "Replace Existing"),

    /** Overwrite global settings with additive mode. The modifier adds to the existing translation, rotation, or scale. */
    Additive UMETA(DisplayName = "Add to Existing")
};

/** Settings for the game which are exposed to the project plugin config screen */
UCLASS(config = Game, defaultconfig)
class FACEFX_API UFaceFXConfig : public UObject
{
    GENERATED_BODY()

public:

    static const UFaceFXConfig& Get()
    {
        const UFaceFXConfig* Instance = GetDefault<UFaceFXConfig>();
        check(Instance);
        return *Instance;
    }

    inline EFaceFXBlendMode GetDefaultBlendMode() const
    {
        return DefaultBlendMode;
    }

private:

    /*
    Default blend mode for the FaceFX runtime evaluation.
This setting determines if the runtime is using absolute (replace mode) or offset (additive mode) transforms
Can be overridden directly via an FaceFXActor asset properties.
    */
    UPROPERTY(config, EditAnywhere, Category = FaceFX)
    EFaceFXBlendMode DefaultBlendMode = EFaceFXBlendMode::Replace;
};
