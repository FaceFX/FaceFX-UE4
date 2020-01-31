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
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "FaceFXAsset.generated.h"

/** Base of all FaceFX assets */
UCLASS(Abstract, hideCategories=Object)
class FACEFX_API UFaceFXAsset : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Checks if this face fx data asset it valid
	* @returns True if valid, else false
	*/
	virtual bool IsValid() const
	{
#if WITH_EDITORONLY_DATA
		return IsAssetPathSet();
#else
		return true;
#endif
	}

#if WITH_EDITORONLY_DATA

	friend struct FFaceFXEditorTools;

	/**
	* Gets the number of animations which are encapsulated in this asset
	* @return The animation count
	*/
	virtual int32 GetAnimationCount() const { check("Not implemented"); return 0; }

	/**
	* Gets the details in a human readable string representation
	* @param OutDetails The resulting details string
	*/
	virtual void GetDetails(FString& OutDetails) const { check("Not implemented"); }

	/**
	* Sets the asset sources
	* @param InAssetName The name of the asset to set
	* @param InAssetFolder The folder path to set
	*/
	inline void SetSources(const FString& InAssetName, const FString& InAssetFolder)
	{
		AssetName = InAssetName;
		AssetFolder = InAssetFolder;
	}

	/**
	* Gets the indicator if the asset paths are set
	* @returns True if both the folder and asset are set
	*/
	inline bool IsAssetPathSet() const
	{
		return !AssetFolder.IsEmpty() && !AssetName.IsEmpty();
	}

	/**
	* Gets the relative path to the source asset
	* @returns The path to the asset
	*/
	inline FString GetAssetPath() const
	{
		return AssetFolder / (AssetName + TEXT(".facefx"));
	}

	/**
	* Gets the absolute path to the source asset
	* @returns The path to the asset
	*/
	FString GetAssetPathAbsolute() const;

	/**
	* Gets the relative source asset folder
	* @returns The relative folder
	*/
	inline const FString& GetAssetFolder() const
	{
		return AssetFolder;
	}

	/**
	* Gets the absolute source asset folder
	* @returns The absolute folder
	*/
	FString GetAssetFolderAbsolute() const;

	/**
	* Gets the source asset name
	* @returns The name
	*/
	inline const FString& GetAssetName() const
	{
		return AssetName;
	}

protected:

	/** The absolute path to the asset source files*/
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FString AssetFolder;

	/** The name of the asset. Used to build filenames (i.e. <name>.facefx)*/
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FString AssetName;

#endif //WITH_EDITORONLY_DATA
};
