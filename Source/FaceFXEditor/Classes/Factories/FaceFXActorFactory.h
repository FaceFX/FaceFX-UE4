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

#include "FaceFXAsset.h"
#include "FaceFXEditorTools.h"
#include "IAssetTypeActions.h"
#include "Factories/Factory.h"
#include "FaceFXActorFactory.generated.h"

class FFeedbackContext;

UCLASS(hidecategories=Object)
class UFaceFXActorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//UFactory
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override
	{
		return FFaceFXEditorTools::AssetCategory;
	}
	virtual FName GetNewAssetThumbnailOverride() const override;
	//~UFactory

	/**
	* Creates a new asset of the given class. This class must be a subclass of UFaceFXAsset
	* @param Class The class to instantiate
	* @param InParent The owning parent object
	* @param Name The name of the new object
	* @param Flags The flags used during instantiation
	* @param BeforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @param FaceFXAsset The filepath of the .facefx asset that shall be imported. Keep empty to have a FileOpenDialog showing up
	* @returns The new object or nullptr if not instantiated
	*/
	static UObject* CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback = FCompilationBeforeDeletionDelegate(), FString FaceFXAsset = "");

	/**
	* Handles the case when a UFaceFXActor asset was created and we want to ask the user to import additional animations
	* @param Asset the asset that got created
	* @param CompilationFolder The compilation folder
	* @param OutResultMessages The result messages
	* @param Factory The factory to pass along
	*/
	static void HandleFaceFXActorCreated(class UFaceFXActor* Asset, const FString& CompilationFolder, FFaceFXImportResult& OutResultMessages, class UFactory* Factory = nullptr);

	/** Callback function for before the compilation folder gets deleted */
	static void OnFxActorCompilationBeforeDelete(class UObject* Asset, const FString& CompilationFolder, bool LoadResult, FFaceFXImportResult& OutResultMessages);
};
