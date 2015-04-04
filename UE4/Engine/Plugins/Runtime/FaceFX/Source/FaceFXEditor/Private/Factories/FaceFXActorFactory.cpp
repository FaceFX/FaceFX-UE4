#include "FaceFXEditor.h"
#include "FaceFX.h"
#include "Factories/FaceFXActorFactory.h"
#include "Factories/FaceFXAnimSetFactory.h"

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

void UFaceFXActorFactory::OnCompilationBeforeDelete(UObject* Asset, const FString& CompilationFolder, bool LoadResult)
{
	if(LoadResult)
	{
		UFaceFXActor* FaceFXActor = CastChecked<UFaceFXActor>(Asset);

		//asset successfully loaded -> ask for importing Animations as well or not
		TArray<FString> AnimGroups;
		if(FFaceFXEditorTools::GetAnimationGroupsInFolder(CompilationFolder, EFaceFXTargetPlatform::PC, AnimGroups))
		{
			if(AnimGroups.Num() > 0)
			{
				FFormatNamedArguments TitleArgs;
				TitleArgs.Add(TEXT("Asset"), FText::FromString(FaceFXActor->GetName()));

				const FText Title = FText::Format(LOCTEXT("ImportFXSetAnimationTitle", "Import Animation Data [{Asset}]"), TitleArgs);

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
					FText::Format(LOCTEXT("ImportFXSetAnimationMessage", "Do you want to automatically import the animation groups and link them to this new asset ?\n\nGroups:\n{Groups}"), Args),
					&Title);

				if(Result == EAppReturnType::Yes)
				{
					FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
					auto& assetTools = AssetToolsModule.Get();

					//generate proper factory
					UFaceFXAnimSetFactory* Factory = ConstructObject<UFaceFXAnimSetFactory>(UFaceFXAnimSetFactory::StaticClass(), this);
					Factory->bOnlyCreate = true;

					FString PackageName;
					Asset->GetOutermost()->GetName(PackageName);

					//import additional animations
					for(const FString& Group : AnimGroups)
					{
						if(!FFaceFXEditorTools::CreateAnimSetAsset(CompilationFolder, Group, PackageName, FaceFXActor, assetTools, Factory))
						{
							FFormatNamedArguments Args;
							Args.Add(TEXT("Asset"), FText::FromString(Group));
							ShowError(FText::Format(LOCTEXT("AnimSetImportFailed", "FaceFX anim set import failed. Asset: {Asset}"), Args));
						}
					}
				}
			}
		}
	}
}

UObject* UFaceFXActorFactory::CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback, FString FaceFXAsset)
{
	if(!FFaceFXEditorTools::IsFaceFXCompilerInstalled())
	{
		ShowError(LOCTEXT("CompilerNotFound", "FaceFX compiler was not found."));
		return nullptr;
	}

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

			//query for files
			const FString Title = LOCTEXT("OpenAssetTitle", "Select FaceFX Asset").ToString();

			TArray<FString> Files;
			DesktopPlatform->OpenFileDialog(ParentWindowWindowHandle, Title, TEXT(""), TEXT(""), TEXT("FaceFX Asset (*.facefx)|*.facefx;"), EFileDialogFlags::None, Files);

			if(Files.Num() <= 0)
			{
				//no file selected
				ShowError(LOCTEXT("CreateFail", "Import of FaceFX asset cancelled."));
				return nullptr;
			}
			FaceFXAsset = Files[0];
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
		UFaceFXAsset* NewAsset = ConstructObject<UFaceFXAsset>(Class, InParent, Name, Flags);

		//initialize asset
		FString ErrorMsg;
		const bool Result = FFaceFXEditorTools::InitializeFromFile(NewAsset, FaceFXAsset, &ErrorMsg, beforeDeletionCallback);
		GWarn->EndSlowTask();

		if(Result)
		{
			//success
			FFaceFXEditorTools::SavePackage(NewAsset->GetOutermost());
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("CreateSuccess", "Import of FaceFX asset succeeded. (%i animations)").ToString(), NewAsset->GetAnimationCount())));
			return NewAsset;
		}
		else
		{
			//initialization failed
			ShowError(FText::FromString(FString::Printf(*LOCTEXT("InitSuccess", "Initialization of FaceFX asset failed. %s").ToString(), *ErrorMsg)));
		}
	}
	else
	{
		ShowError(LOCTEXT("CreateFail", "Import failed. FaceFX asset missing."));
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE