/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015 OC3 Entertainment, Inc.
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

#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "Factories/FaceFXActorFactory.h"
#include "Factories/FaceFXAnimFactory.h"

#include "AssetToolsModule.h"
#include "EditorStyleSet.h"
#include "IMainFrameModule.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "ObjectTools.h"
#include "ISourceControlModule.h"
#include "Editor.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "Include/Slate/FaceFXResultWidget.h"

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

/**
* Shows an error notification message
* @param msg The message to show
*/
void ShowError(const FText& Msg)
{
	FNotificationInfo Info(Msg);
	Info.ExpireDuration = 10.F;
	Info.bUseLargeFont = false;
	Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

/**
* Shows an info notification message
* @param msg The message to show
*/
void ShowInfo(const FText& Msg)
{
	FNotificationInfo Info(Msg);
	Info.ExpireDuration = 10.F;
	Info.bUseLargeFont = false;
	Info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

UObject* UFaceFXActorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return CreateNew(InClass, InParent, InName, Flags, FCompilationBeforeDeletionDelegate::CreateUObject(this, &UFaceFXActorFactory::OnCompilationBeforeDelete), GetCurrentFilename());
}

void UFaceFXActorFactory::OnCompilationBeforeDelete(UObject* Asset, const FString& CompilationFolder, bool LoadResult, FFaceFXImportResult& OutResultMessages)
{
	if(LoadResult && FFaceFXEditorTools::IsImportAnimationOnActorImport())
	{
		//generate proper factory
		UFaceFXAnimFactory* Factory = NewObject<UFaceFXAnimFactory>(UFaceFXAnimFactory::StaticClass());
		check(Factory);
		Factory->bOnlyCreate = true;

		//prevent GC
		FScopedObjectRoot ScopedRoot(Factory);

		HandleFaceFXActorCreated(CastChecked<UFaceFXActor>(Asset), CompilationFolder, OutResultMessages, Factory);
	}
}

void UFaceFXActorFactory::HandleFaceFXActorCreated(class UFaceFXActor* Asset, const FString& CompilationFolder, FFaceFXImportResult& OutResultMessages, UFactory* Factory)
{
	check(Asset);

	if(!FFaceFXEditorTools::IsImportAnimationOnActorImport())
	{
		//animation import disabled
		return;
	}

	//asset successfully loaded -> ask for importing Animations as well or not
	TArray<FString> AnimGroups;
	if(FFaceFXEditorTools::GetAnimationGroupsInFolder(CompilationFolder, EFaceFXTargetPlatform::PC, &AnimGroups))
	{
		if(AnimGroups.Num() > 0)
		{
			const FText Title = FText::Format(LOCTEXT("AutoImportAnimationTitle", "Import Animation Data [{0}]"), FText::FromString(Asset->GetName()));

			//collect all groups
			FString Groups;
			for(auto& Group : AnimGroups)
			{
				Groups += Group + TEXT("\n");
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("Groups"), FText::FromString(Groups));

			//we have animations -> ask user if he wants to import and link those into new assets. One per group
			const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
				FText::Format(LOCTEXT("AutoImportAnimationMessage", "Do you want to automatically (re)import the existing animations for the following groups ?\n\nGroups:\n{Groups}"), Args),
				&Title);

			if(Result == EAppReturnType::Yes)
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
	}
}

/**
* Performs some steps during asset import after the asset got create and before the actual import starts. Gives a chance to prepare the asset and adjust the asset to load from
* @param Asset The asset that got created
* @param OutFaceFXAsset The asset import string
*/
void OnPreInitialization(UFaceFXAsset* Asset, FString& OutFaceFXAsset)
{
	UFaceFXAnim* AnimAsset = Cast<UFaceFXAnim>(Asset);
	if(AnimAsset && OutFaceFXAsset.EndsWith(FACEFX_FILEEXT_ANIM))
	{
		//we actually directly import from an animation asset instead from the .fxactor root folder -> transform back to the root folder and set asset ids
		//C:\_FaceFX_Compiled_Wav\Slade-UE4.ffxc\x86\Text - No Tweaking/the-proposal-001-CHUBUKOV.ffxc
		//C:/_FaceFX_Compiled_Wav/Slade-UE4.ffxc
		const FString GroupPath = FPaths::GetPath(OutFaceFXAsset);
		const FString AnimName = FPaths::GetBaseFilename(OutFaceFXAsset);
		const FString AnimGroup = FPaths::GetBaseFilename(GroupPath);
		FFaceFXAnimId& AssetAnimId = AnimAsset->GetId();
		AssetAnimId.Group = FName(*AnimGroup);
		AssetAnimId.Name = FName(*AnimName);

		const FString CompilationFolder = FPaths::GetPath(FPaths::GetPath(GroupPath));
		const FString CompilationFolderName = FPaths::GetBaseFilename(CompilationFolder);

		//create the path to the .fxactor file. The asset knows know what animation/group the load process shall target so we can use the more generic .fxactor approach now
		OutFaceFXAsset = CompilationFolder / TEXT("../") + CompilationFolderName + FACEFX_FILEEXT_FACEFX;
	}
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
				ShowError(LOCTEXT("CreateFail", "Import of FaceFX asset cancelled."));
				return nullptr;
			}
			FaceFXAsset = Files[0];
			LastCreatePath = FaceFXAsset;
		}
		else
		{
			ShowError(LOCTEXT("CreateFail", "Internal Error: Retrieving platform module failed."));
			return nullptr;
		}
	}

	if(!FaceFXAsset.IsEmpty())
	{
		GWarn->BeginSlowTask(LOCTEXT("ImportProgress", "Importing FaceFX Assets..."), true);

		//create asset
		UFaceFXAsset* NewAsset = NewObject<UFaceFXAsset>(InParent, Class, Name, Flags);

		OnPreInitialization(NewAsset, FaceFXAsset);

		//initialize asset
		FFaceFXImportResultSet ResultSet;
		
		if(FFaceFXEditorTools::InitializeFromFile(NewAsset, FaceFXAsset, ResultSet.GetOrAdd(NewAsset), BeforeDeletionCallback, true))
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
		ShowError(LOCTEXT("CreateFail", "Import failed. FaceFX asset missing."));
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE