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

#include "Factories/FaceFXActorFactory.h"
#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "Factories/FaceFXAnimFactory.h"
#include "Include/Slate/FaceFXStyle.h"
#include "Include/Slate/FaceFXResultWidget.h"
#include "FaceFXEditorTools.h"
#include "FaceFXEditorConfig.h"
#include "AssetToolsModule.h"
#include "EditorStyleSet.h"
#include "Interfaces/IMainFrameModule.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "ObjectTools.h"
#include "ISourceControlModule.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"


#define LOCTEXT_NAMESPACE "FaceFX"

UFaceFXActorFactory::UFaceFXActorFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	SupportedClass = UFaceFXActor::StaticClass();
	bCreateNew = true;
	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("facefx;FaceFX Asset"));
}

FName UFaceFXActorFactory::GetNewAssetThumbnailOverride() const
{
	return FFaceFXStyle::GetBrushIdFxActor();
}

UObject* UFaceFXActorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return CreateNew(InClass, InParent, InName, Flags, FCompilationBeforeDeletionDelegate::CreateStatic(&UFaceFXActorFactory::OnFxActorCompilationBeforeDelete), GetCurrentFilename());
}

void UFaceFXActorFactory::OnFxActorCompilationBeforeDelete(UObject* Asset, const FString& CompilationFolder, bool LoadResult, FFaceFXImportResult& OutResultMessages)
{
	if(LoadResult && UFaceFXEditorConfig::Get().IsImportAnimationOnActorImport())
	{
		//generate proper factory
		UFaceFXAnimFactory* Factory = NewObject<UFaceFXAnimFactory>(UFaceFXAnimFactory::StaticClass());
		check(Factory);

		//prevent GC
		FScopedObjectRoot ScopedRoot(Factory);

		HandleFaceFXActorCreated(CastChecked<UFaceFXActor>(Asset), CompilationFolder, OutResultMessages, Factory);
	}
}

void UFaceFXActorFactory::HandleFaceFXActorCreated(UFaceFXActor* Asset, const FString& CompilationFolder, FFaceFXImportResult& OutResultMessages, UFactory* Factory)
{
	check(Asset);

	if(!UFaceFXEditorConfig::Get().IsImportAnimationOnActorImport())
	{
		//animation import disabled
		return;
	}

	//asset successfully loaded -> ask for importing Animations as well or not
	TArray<FString> AnimGroups;
	if(FFaceFXEditorTools::GetAnimationGroupsInFolder(CompilationFolder, &AnimGroups) && AnimGroups.Num() > 0)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		FString PackageName;
		Asset->GetOutermost()->GetName(PackageName);

		//import additional animations
		for(const FString& Group : AnimGroups)
		{
			FFaceFXEditorTools::ReimportOrCreateAnimAssets(CompilationFolder, Group, PackageName, Asset, AssetTools, OutResultMessages, Factory);
		}
	}
}

/**
* Performs some steps during asset import after the asset got created and before the actual import starts. Gives a chance to prepare the asset and adjust the asset to load from
* @param Asset The asset that got created
* @param OutFaceFXAsset The asset import string
* @param OutResult The result messages
* @returns True if succeeded, else false
*/
bool OnPreInitialization(UFaceFXAsset* Asset, FString& OutFaceFXAsset, FFaceFXImportResultSet& OutResult)
{
	return true;
}

UObject* UFaceFXActorFactory::CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& BeforeDeletionCallback, FString FaceFXAsset)
{
	if(FaceFXAsset.IsEmpty())
	{
		//fetch source file
		if(IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
		{
			//get parent window
			void* ParentWindowWindowHandle = nullptr;
			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
			if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
			{
				ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			static FString LastCreatePath;

			//query for files
			TArray<FString> Files;
			DesktopPlatform->OpenFileDialog(ParentWindowWindowHandle, LOCTEXT("OpenAssetTitle", "Select FaceFX Asset").ToString(), TEXT(""), LastCreatePath, FACEFX_FILEFILTER_ASSET_ACTOR, EFileDialogFlags::None, Files);

			if(Files.Num() <= 0)
			{
				//no file selected
				FFaceFXEditorTools::ShowError(LOCTEXT("CreateFail", "Import of FaceFX asset cancelled."));
				return nullptr;
			}
			FaceFXAsset = Files[0];
			LastCreatePath = FaceFXAsset;
		}
		else
		{
			FFaceFXEditorTools::ShowError(LOCTEXT("CreateFail", "Internal Error: Retrieving platform module failed."));
			return nullptr;
		}
	}

	if(!FaceFXAsset.IsEmpty())
	{
		GWarn->BeginSlowTask(LOCTEXT("ImportProgress", "Importing FaceFX Assets..."), true);

		//create asset
		UFaceFXAsset* NewAsset = NewObject<UFaceFXAsset>(InParent, Class, Name, Flags);

		//initialize asset
		FFaceFXImportResultSet ResultSet;

		if(OnPreInitialization(NewAsset, FaceFXAsset, ResultSet) && FFaceFXEditorTools::InitializeFromFile(NewAsset, FaceFXAsset, ResultSet.GetOrAdd(NewAsset), BeforeDeletionCallback, true))
		{
			//success
			FFaceFXEditorTools::SavePackage(NewAsset->GetOutermost());
		}

		GWarn->EndSlowTask();

		FFaceFXResultWidget::Create(LOCTEXT("ShowCreateFxActorResultTitle", "Create FaceFX Asset Result"), ResultSet);

		return NewAsset;
	}
	else
	{
		FFaceFXEditorTools::ShowError(LOCTEXT("CreateFail", "Import failed. FaceFX asset missing."));
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
