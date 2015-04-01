#pragma once

#include "FaceFxAsset.generated.h"

/** Base of all FaceFX assets */
UCLASS(Abstract, hideCategories=Object)
class FACEFX_API UFaceFxAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR

	friend struct FFaceFxEditorTools;

	/**
	* Gets the number of animations which are encapsulated in this asset
	* @return The animation count
	*/
	virtual int32 GetAnimationCount() const { checkf("Not implemented"); return 0; }

	/**
	* Gets the details in a human readable string representation
	* @param outDetails The resulting details string
	*/
	virtual void GetDetails(FString& outDetails) const { checkf("Not implemented"); }

	/**
	* Sets the asset sources
	* @param assetName The name of the asset
	* @param folder The folder path
	*/
	inline void SetSources(const FString assetName, const FString& folder)
	{
		m_assetName = assetName;
		m_assetFolder = folder;
	}

#endif

	/**
	* Checks if this face fx data asset it valid
	* @returns True if valid, else false
	*/
	virtual bool IsValid() const
	{
		return IsAssetPathSet();
	}

	/**
	* Gets the indicator if the asset paths are set
	* @returns True if both the folder and asset are set
	*/
	inline bool IsAssetPathSet() const
	{
		return !m_assetFolder.IsEmpty() && !m_assetName.IsEmpty();
	}

	/** 
	* Gets the relative path to the source asset
	* @returns The path to the asset
	*/
	inline FString GetAssetPath() const
	{
		return m_assetFolder / (m_assetName + TEXT(".facefx"));
	}

	/** 
	* Gets the absolute path to the source asset
	* @returns The path to the asset
	*/
	inline FString GetAssetPathAbsolute() const
	{
		return FPaths::ConvertRelativePathToFull(GetAssetPath());
	}

	/**
	* Gets the relative source asset folder
	* @returns The relative folder
	*/
	inline const FString& GetAssetFolder() const
	{
		return m_assetFolder;
	}

	/**
	* Gets the absolute source asset folder
	* @returns The absolute folder
	*/
	inline FString GetAssetFolderAbsolute() const
	{
		return FPaths::ConvertRelativePathToFull(m_assetFolder);
	}

	/**
	* Gets the source asset name
	* @returns The name
	*/
	inline const FString& GetAssetName() const
	{
		return m_assetName;
	}

protected:

	/** The absolute path to the asset source files*/
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FString m_assetFolder;

	/** The name of the asset. Used to build filenames (i.e. <name>.facefx)*/
	UPROPERTY(EditInstanceOnly, Category=FaceFX)
	FString m_assetName;

#if WITH_EDITOR

	/** 
	* Clear platform specific data based on the target Archive platform
	* @param Ar The archive to use
	* @param platformData The data to clear
	*/
	template <typename T> void ClearPlatformData(const class FArchive& Ar, T& platformData);

#endif
};