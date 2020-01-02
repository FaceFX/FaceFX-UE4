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

#include "FaceFXConfig.h"
#include "FaceFXAsset.h"
#include "FaceFXData.h"
#include "FaceFXActor.generated.h"

/** Blend mode for FaceFX runtime that is set per actor asset */
UENUM()
enum class EFaceFXActorBlendMode : uint8
{
	/** The global settings are used as blend mode. See FaceFXConfig.h, FACEFX_DEFAULT_BLEND_MODE */
	Global UMETA(DisplayName = "Use Global Config"),

	/** Overwrite global settings with replace mode. The modifier replaces the existing translation, rotation, or scale. */
	Replace UMETA(DisplayName = "Replace Existing"),

	/** Overwrite global settings with additive mode. The modifier adds to the existing translation, rotation, or scale. */
	Additive UMETA(DisplayName = "Add to Existing")
};

/** Asset that can be assigned to FaceFXComponents and which contain the FaceFX runtime data */
UCLASS(BlueprintType, hideCategories=Object)
class FACEFX_API UFaceFXActor : public UFaceFXAsset
{
	GENERATED_UCLASS_BODY()

public:

	//UObject
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~UObject

	virtual bool IsValid() const override
	{
		return Super::IsValid() && ActorData.IsValid();
	}

	inline EFaceFXActorBlendMode GetBlendMode() const
	{
		return BlendMode;
	}

#if WITH_EDITORONLY_DATA

	friend struct FFaceFXEditorTools;

	//UObject
	virtual void Serialize(FArchive& Ar) override;
	//~UObject

	/**
	* Gets the details in a human readable string representation
	* @param OutDetails The resulting details string
	*/
	virtual void GetDetails(FString& OutDetails) const override;

	inline void Reset()
	{
		ActorData.Reset();
	}

#endif //WITH_EDITORONLY_DATA

#if WITH_EDITORONLY_DATA && FACEFX_USEANIMATIONLINKAGE
	inline void LinkTo(class UFaceFXAnim* AnimSet)
	{
		Animations.AddUnique(AnimSet);
	}

	inline bool UnlinkFrom(class UFaceFXAnim* AnimSet)
	{
		return Animations.Remove(AnimSet) > 0;
	}

	virtual int32 GetAnimationCount() const override;

#endif //WITH_EDITOR && FACEFX_USEANIMATIONLINKAGE

	inline FFaceFXActorData& GetData()
	{
		return ActorData;
	}

	inline const FFaceFXActorData& GetData() const
	{
		return ActorData;
	}

#if FACEFX_USEANIMATIONLINKAGE
	const class UFaceFXAnim* GetAnimation(const FName& AnimGroup, const FName& AnimName) const;

	inline const class UFaceFXAnim* GetAnimation(const FFaceFXAnimId& AnimId) const
	{
		return GetAnimation(AnimId.Group, AnimId.Name);
	}

	void GetAnimationGroups(TArray<FName>& OutGroups) const;

	void GetAnimationIds(TArray<FFaceFXAnimId>& OutAnimIds) const;

#endif //FACEFX_USEANIMATIONLINKAGE

private:

	UPROPERTY()
	TArray<FFaceFXActorData> PlatformData_DEPRECATED;

	/** The actor data. */
	UPROPERTY(VisibleInstanceOnly, Category=FaceFX)
	FFaceFXActorData ActorData;

	/** The linked animations where this set look up the animations in */
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	TArray<class UFaceFXAnim*> Animations;

	/** The linked animations where this set look up the animations in */
	UPROPERTY(EditAnywhere, Category = FaceFX)
	EFaceFXActorBlendMode BlendMode = EFaceFXActorBlendMode::Global;
};
