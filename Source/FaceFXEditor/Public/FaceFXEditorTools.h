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

#include "FaceFXData.h"
#include "FaceFXEditor.h"
#include "Misc/Paths.h"

#include "FaceFXEditorTools.generated.h"

/** A struct that represents an action performed during an import process and its result */
struct FFaceFXImportActionResult
{
	friend struct FFaceFXImportResult;

	/** The import action type*/
	enum class ActionType
	{
		None,
		Create,
		Modify,
	};

	/** The result type */
	enum class ResultType
	{
		Success,
		Warning,
		Error,
	};

	FFaceFXImportActionResult() : Type(ActionType::None), Result(ResultType::Error) {}
	FFaceFXImportActionResult(ActionType InType, ResultType InResult, const TSoftObjectPtr<class UFaceFXAsset>& InImportAsset = nullptr, const FText& InMessage = FText::GetEmpty()) :
		Type(InType), Result(InResult), Message(InMessage), ImportAsset(InImportAsset) {}

	/**
	* Rolls this action back
	* @returns True if successfully rolled back, else false
	*/
	bool Rollback();

	/**
	* Gets the indicator if the result can be rolled back
	* @returns True if rollback'able, else false
	*/
	inline bool CanRollback() const
	{
		return Type == ActionType::Create;
	}

	/**
	* Gets the action type assigned to this result
	* @returns The action
	*/
	inline ActionType GetType() const
	{
		return Type;
	}

	/**
	* Gets the result type assigned to this result
	* @returns The action
	*/
	inline ResultType GetResultType() const
	{
		return Result;
	}

	/**
	* Gets the assigned asset
	* @returns The asset
	*/
	inline const TSoftObjectPtr<UObject>& GetAsset() const
	{
		return Asset;
	}

	/**
	* Sets the assigned asset
	* @param InAsset The new asset
	*/
	inline void SetAsset(const TSoftObjectPtr<UObject>& InAsset)
	{
		Asset = InAsset;
	}


	/**
	* Gets the assigned import asset
	* @returns The import asset
	*/
	inline const TSoftObjectPtr<UFaceFXAsset>& GetImportAsset() const
	{
		return ImportAsset;
	}

	/**
	* Gets the message assigned to this result
	* @returns The message
	*/
	inline const FText& GetMessage() const
	{
		return Message;
	}

private:

	/**  The assigned action type */
	ActionType Type;

	/** The assigned result type*/
	ResultType Result;

	/** The message assigned to this result */
	FText Message;

	/** The affected asset */
	TSoftObjectPtr<UObject> Asset;

	/** The asset for which the action was performed */
	TSoftObjectPtr<class UFaceFXAsset> ImportAsset;
};

/** A whole result set for a complete import process */
USTRUCT()
struct FFaceFXImportResult
{
	GENERATED_USTRUCT_BODY()

	FFaceFXImportResult(const TSoftObjectPtr<class UFaceFXAsset>& InImportRootAsset = nullptr) : ImportRootAsset(InImportRootAsset) {}

	/**
	* Adds a new message to the result set
	* @param Message The message to add
	* @param Type The action type
	* @param Result The result type
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& Add(const FText& Message, FFaceFXImportActionResult::ActionType Type, FFaceFXImportActionResult::ResultType Result, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		FFaceFXImportActionResult NewEntry = FFaceFXImportActionResult(Type, Result, ImportAsset, Message);
		NewEntry.Asset = InAsset.ToSoftObjectPath();
		return Entries[Entries.Add(NewEntry)];
	}

	/**
	* Adds a new error message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddModifyError(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Modify, FFaceFXImportActionResult::ResultType::Error, ImportAsset, InAsset);
	}

	/**
	* Adds a new error message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddCreateError(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Create, FFaceFXImportActionResult::ResultType::Error, ImportAsset, InAsset);
	}

	/**
	* Adds a new warning message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddModifyWarning(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Modify, FFaceFXImportActionResult::ResultType::Warning, ImportAsset, InAsset);
	}

	/**
	* Adds a new warning message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddCreateWarning(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Create, FFaceFXImportActionResult::ResultType::Warning, ImportAsset, InAsset);
	}

	/**
	* Adds a new success message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddModifySuccess(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Modify, FFaceFXImportActionResult::ResultType::Success, ImportAsset, InAsset);
	}

	/**
	* Adds a new success message to the result set
	* @param Message The message to add
	* @param ImportAsset The imported asset
	* @param InAsset The affected asset
	* @returns The added result entry
	*/
	template <typename TAssetType = UObject>
	FFaceFXImportActionResult& AddCreateSuccess(const FText& Message, const TSoftObjectPtr<class UFaceFXAsset>& ImportAsset = nullptr, const TSoftObjectPtr<TAssetType>& InAsset = nullptr)
	{
		return Add(Message, FFaceFXImportActionResult::ActionType::Create, FFaceFXImportActionResult::ResultType::Success, ImportAsset, InAsset);
	}

	/**
	* Gets the entries
	* @returns The entries
	*/
	inline const TArray<FFaceFXImportActionResult>& GetEntries() const
	{
		return Entries;
	}

	/**
	* Gets the import root asset
	* @returns The root asset
	*/
	inline const TSoftObjectPtr<class UFaceFXAsset>& GetImportRootAsset() const
	{
		return ImportRootAsset;
	}

	/**
	* Gets the maximum error level for all result entries in the set
	* @returns The maximum error level
	*/
	inline FFaceFXImportActionResult::ResultType GetMaxErrorLevel() const
	{
		FFaceFXImportActionResult::ResultType Result = FFaceFXImportActionResult::ResultType::Success;
		for(const FFaceFXImportActionResult& Entry : Entries)
		{
			const FFaceFXImportActionResult::ResultType EntryResult = Entry.GetResultType();
			if(Result < EntryResult)
			{
				Result = EntryResult;
				if(Result >= FFaceFXImportActionResult::ResultType::Error)
				{
					break;
				}
			}
		}
		return Result;
	}

	FORCEINLINE bool operator==(const TSoftObjectPtr<class UFaceFXAsset>& Asset) const
	{
		return ImportRootAsset == Asset;
	}

private:

	/** All the result data for actions performed during an import process */
	TArray<FFaceFXImportActionResult> Entries;

	/** The root action that initially requested the import */
	TSoftObjectPtr<class UFaceFXAsset> ImportRootAsset;
};

/** A list of multiple import results per asset */
struct FFaceFXImportResultSet
{
	/**
	* Gets or creates a new entry
	* @param Asset The current root asset
	* @returns The new or existing entry
	*/
	inline FFaceFXImportResult& GetOrAdd(const TSoftObjectPtr<class UFaceFXAsset>& Asset)
	{
		if(FFaceFXImportResult* Entry = Entries.FindByKey(Asset))
		{
			return *Entry;
		}
		return Entries[Entries.Add(FFaceFXImportResult(Asset))];
	}

	inline const FFaceFXImportResult* GetResult(const TSoftObjectPtr<class UFaceFXAsset>& Asset) const
	{
		return Entries.FindByKey(Asset);
	}

	/**
	* Gets all entries
	* @returns The entries
	*/
	inline const TArray<FFaceFXImportResult>& GetEntries() const
	{
		return Entries;
	}

	/**
	* Gets the indicator if this result set contains at least one error
	* @returns True if one or more error are inside this result set. Else false
	*/
	inline bool IsContainError() const
	{
		return GetMaxErrorLevel() == FFaceFXImportActionResult::ResultType::Error;
	}

	/**
	* Gets the maximum error level for all result entries in the set
	* @returns The maximum error level
	*/
	inline FFaceFXImportActionResult::ResultType GetMaxErrorLevel() const
	{
		FFaceFXImportActionResult::ResultType Result = FFaceFXImportActionResult::ResultType::Success;
		for(const FFaceFXImportResult& Entry : Entries)
		{
			const FFaceFXImportActionResult::ResultType EntryResult = Entry.GetMaxErrorLevel();
			if(Result < EntryResult)
			{
				Result = EntryResult;
				if(Result >= FFaceFXImportActionResult::ResultType::Error)
				{
					break;
				}
			}
		}
		return Result;
	}

private:

	/** All entries */
	TArray<FFaceFXImportResult> Entries;
};

/** A helper struct that turns objects into rooted objects for the duration of the scope */
struct FScopedObjectRoot
{
	FScopedObjectRoot(UObject* InObj) : Obj(nullptr)
	{
		if (InObj && !InObj->IsRooted())
		{
			Obj = InObj;
			Obj->AddToRoot();
		}
	}

	~FScopedObjectRoot()
	{
		if (Obj)
		{
			Obj->RemoveFromRoot();
		}
	}
private:

	UObject* Obj;
};

/** The callback for when assets get imported and we want to do something before the compiled data gets deleted */
DECLARE_DELEGATE_FourParams(FCompilationBeforeDeletionDelegate, UObject* /** Asset */, const FString& /* CompilationFolder */, bool /** ImportResult */, FFaceFXImportResult& /** ResultMessages */);

/** Editor specific FaceFX functions */
struct FACEFXEDITOR_API FFaceFXEditorTools
{
	/** The custom asset category ID */
	static uint32 AssetCategory;

	/**
	* Checks if FaceFX studio is installed within the standard installation path
	* @returns True if installed, else false
	*/
	static bool IsFaceFXStudioInstalled();

	/**
	* Opens the given FaceFX asset within FaceFX studio in case this is installed locally
	* @param Asset The asset to open
	* @param OutErrorMessage The optional error message in case it failed
	* @returns True if open succeeded, else false
	*/
	static bool OpenFaceFXStudio(class UFaceFXActor* Asset, FString* OutErrorMessage = nullptr);

	/**
	* Reimports a given FaceFX asset
	* @param Asset The target asset to reimport
	* @param OutErrorMessage The optional error message in case it failed
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ReImportFaceFXAsset(class UFaceFXAsset* Asset, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Imports a given FaceFX asset
	* @param Asset The target asset to import into
	* @param AssetPath The path to the .facefx asset file
	* @param OutResultMessages The result set
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ImportFaceFXAsset(class UFaceFXAsset* Asset, const FString& AssetPath, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Gets all animation groups found within a given FaceFX compilation folder
	* @param Folder The compilation folder to load from
	* @param OutGroups The optional resulting animation groups
	* @param OutAnimGroupIds The optional <group.animation> id list
	* @returns True if at least one group exist that contains .ffxanim files, else false
	*/
	static bool GetAnimationGroupsInFolder(const FString& Folder, TArray<FString>* OutGroups = nullptr, TArray<FString>* OutAnimGroupIds = nullptr);

	/**
	* Gets the FaceFX studio output folder for a given .facefx file
	* @param FaceFXFile The path to the .facefx file
	* @returns The compiler output folder
	*/
	static inline FString GetCompilationFolder(const FString& FaceFXFile)
	{
		const FString Filename = FPaths::GetBaseFilename(FaceFXFile);
		return FPaths::GetPath(FaceFXFile) / (Filename + TEXT(".ffxc"));
	}

	/**
	* Reimport an existing asset or creates a new anim asset
	* @param CompilationFolder The folder to load the data from
	* @param AnimGroup The animation group where to read and create the animation data from
	* @param PackageName The name of the package
	* @param FaceFXActor The FaceFX actor asset that shall be linked to that new asset
	* @param AssetTools The asset tools instance to use
	* @param OutResultMessages The result set
	* @param Factory The factory to use. Keep at nullptr to directly create an instance
	* @param OutResult The created assets
	* @returns True if succeeded, else if at least one failed
	*/
	static bool ReimportOrCreateAnimAssets(const FString& CompilationFolder, const FString& AnimGroup, const FString& PackageName, class UFaceFXActor* FaceFXActor, class IAssetTools& AssetTools, FFaceFXImportResult& OutResultMessages, class UFactory* Factory = nullptr, TArray<class UFaceFXAnim*>* OutResult = nullptr);

	/**
	* Reimport an existing asset or creates a new anim asset
	* @param CompilationFolder The folder to load the data from
	* @param AnimGroup The animation group where the given file
	* @param AnimFile The animation asset file to load the data from
	* @param PackageName The name of the package
	* @param FaceFXActor The FaceFX actor asset that shall be linked to that new asset
	* @param AssetTools The asset tools instance to use
	* @param OutResultMessages The result set
	* @param Factory The factory to use. Keep at nullptr to directly create an instance
	* @returns The created asset, nullptr if failed
	*/
	static class UFaceFXAnim* ReimportOrCreateAnimAsset(const FString& CompilationFolder, const FString& AnimGroup, const FString& AnimFile, const FString& PackageName, class UFaceFXActor* FaceFXActor, class IAssetTools& AssetTools, FFaceFXImportResult& OutResultMessages, class UFactory* Factory = nullptr);

	/**
	* Performs steps to store an asset physically and in source control
	* @param Package The package to save
	* @param addToSc Indicator if the package shall be added to the source control provider if not done yet
	*/
	static void SavePackage(UPackage* Package, bool addToSc = true);

	/**
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutResultMessages The result set
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAsset* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutResultMessages The result set
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXActor* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder
	* @param Asset The target asset
	* @param Folder The path to the folder to load the compiled assets from
	* @param OutResultMessages The result set
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAnim* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Initializes the asset from a .facefx asset file
	* @param Asset The target asset
	* @param File The path to the .facefx asset file
	* @param OutResultMessages The result set
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @param IsAssetNew Indicator that defines if the given Asset was created new and the init calls is the initial init call on that asset
	* @returns True if succeeded, else false
	*/
	static bool InitializeFromFile(class UFaceFXAsset* Asset, const FString& File, FFaceFXImportResult& OutResultMessages, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate(), bool IsAssetNew = false);

	/**
	* Sets the focus on a given asset within the content browser
	* @param Asset The asset to focus
	*/
	static void ContentBrowserFocusAsset(const FSoftObjectPath& Asset);

	/**
	* Deletes the gives asset
	* @param Asset The asset to delete
	*/
	static void DeleteAsset(UObject* Asset);

	/**
	* Shows a slate error message
	* @param msg The error message to show
	*/
	static void ShowError(const FText& Msg);

	/**
	* Shows a slate info message
	* @param msg The info message to show
	*/
	static void ShowInfo(const FText& Msg);

private:

	FFaceFXEditorTools(){}

	/**
	* Reloads the data from from a facefx compilation folder. Doesn't change asset name and folder. Assigns a new group before doing so
	* @param Asset The target asset
	* @param Group The group to use when loading the animation
	* @param Animation The animation to use when loading the animation
	* @param Folder The path to the platform folder to load the compiled assets from
	* @param OutResultMessages The result set
	* @returns True if succeeded, else false
	*/
	static bool LoadFromCompilationFolder(class UFaceFXAnim* Asset, const FName& Group, const FName& Animation, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Loads all actor data from a given folder into the given asset
	* @param Asset The target asset
	* @param Folder The path to the folder to load the compiled data from
	* @param OutResultMessages The result set
	* @returns True if succeeded, else false
	*/
	static bool LoadCompiledActorData(UFaceFXActor* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Loads the audio file that is mapped to the given asset within the audio map file in the given folder and links to it. Creates a new USound asset if needed
	* @param Asset The asset to create and link the audio for
	* @param Folder The folder to load the audio map file from
	* @param OutResultMessages The result set
	* @returns True if succeeded (no audio set or link/create succeeded), else false
	*/
	static bool LoadAudio(class UFaceFXAnim* Asset, const FString& Folder, FFaceFXImportResult& OutResultMessages);

	/**
	* Locates the USoundWave asset that was generated out of the given audio source file
	* @param AudioSourceFile The absolute source file path to the audio file
	* @returns The asset that was generated using that audio source or unassigned TSoftObjectPtr if not found
	*/
	static TSoftObjectPtr<class USoundWave> LocateAudio(const FString& AudioSourceFile);
};
