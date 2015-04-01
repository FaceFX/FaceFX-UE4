#include "FaceFxEditor.h"
#include "Factories/FaceFxActorFactory.h"

#include "FaceFx.h"
#include "Factories/FaceFxAnimSetFactory.h"

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

#define LOCTEXT_NAMESPACE "FaceFx"

UFaceFxActorFactory::UFaceFxActorFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	SupportedClass = UFaceFxActor::StaticClass();
	bCreateNew = true;
	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("facefx;FaceFX Asset"));
}

/**
* Shows an error notification message
* @param msg The message to show
*/
void ShowError(const FText& msg)
{
	FNotificationInfo info(msg);
	info.ExpireDuration = 10.F;
	info.bUseLargeFont = false;
	info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
	FSlateNotificationManager::Get().AddNotification(info);
}

/**
* Shows an info notification message
* @param msg The message to show
*/
void ShowInfo(const FText& msg)
{
	FNotificationInfo info(msg);
	info.ExpireDuration = 10.F;
	info.bUseLargeFont = false;
	info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
	FSlateNotificationManager::Get().AddNotification(info);
}

UObject* UFaceFxActorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return CreateNew(InClass, InParent, InName, Flags, FCompilationBeforeDeletionDelegate::CreateUObject(this, &UFaceFxActorFactory::OnCompilationBeforeDelete), GetCurrentFilename());
}

/** Callback function for before the compilation folder gets deleted */
void UFaceFxActorFactory::OnCompilationBeforeDelete(UObject* asset, const FString& compilationFolder, bool loadResult)
{
	if(loadResult)
	{
		UFaceFxActor* fxActor = CastChecked<UFaceFxActor>(asset);

		//asset successfully loaded -> ask for importing Animations as well or not
		TArray<FString> animGroups;
		if(FFaceFxEditorTools::GetAnimationGroupsInFolder(compilationFolder, EFaceFxTargetPlatform::PC, animGroups))
		{
			if(animGroups.Num() > 0)
			{
				FFormatNamedArguments titleArgs;
				titleArgs.Add(TEXT("Asset"), FText::FromString(fxActor->GetName()));

				const FText title = FText::Format(LOCTEXT("ImportFxSetAnimationTitle", "Import Animation Data [{Asset}]"), titleArgs);

				//collect all groups
				FString groups;
				for(auto& group : animGroups)
				{
					groups += group + TEXT("\n");
				}

				FFormatNamedArguments args;
				args.Add(TEXT("Groups"), FText::FromString(groups));
				
				//we have animations -> ask user if he wants to import and link those into new assets. One per group
				const EAppReturnType::Type result = FMessageDialog::Open(EAppMsgType::YesNo,
					FText::Format(LOCTEXT("ImportFxSetAnimationMessage", "Do you want to automatically import the animation groups and link them to this new asset ?\n\nGroups:\n{Groups}"), args),
					&title);

				if(result == EAppReturnType::Yes)
				{
					FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
					auto& assetTools = assetToolsModule.Get();

					//generate proper factory
					UFaceFxAnimSetFactory* factory = ConstructObject<UFaceFxAnimSetFactory>(UFaceFxAnimSetFactory::StaticClass(), this);
					factory->m_onlyCreate = true;

					FString packageName;
					asset->GetOutermost()->GetName(packageName);

					//import additional animations
					for(const FString& group : animGroups)
					{
						if(!FFaceFxEditorTools::CreateAnimSetAsset(compilationFolder, group, packageName, fxActor, assetTools, factory))
						{
							FFormatNamedArguments args;
							args.Add(TEXT("Asset"), FText::FromString(group));
							ShowError(FText::Format(LOCTEXT("AnimSetImportFailed", "FaceFX anim set import failed. Asset: {Asset}"), args));
						}
					}
				}
			}
		}
	}
}

/**
* Creates a new asset of the given class. This class must be a subclass of UYFaceFxAsset
* @param Class The class to instantiate
* @param InParent The owning parent object
* @param Name The name of the new object
* @param Flags The flags used during instantiation
* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
* @param FaceFxAsset The filepath of the .facefx asset that shall be imported. Keep empty to have a FileOpenDialog showing up
* @returns The new object or nullptr if not instantiated
*/
UObject* UFaceFxActorFactory::CreateNew(UClass* Class, UObject* InParent, const FName& Name, EObjectFlags Flags, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback, FString FaceFxAsset)
{
	if(!FFaceFxEditorTools::IsFaceFXCompilerInstalled())
	{
		ShowError(LOCTEXT("CompilerNotFound", "FaceFX compiler was not found."));
		return nullptr;
	}

	if(FaceFxAsset.IsEmpty())
	{
		//fetch source file
		if(IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get())
		{
			//get parent window
			void* parentWindowWindowHandle = nullptr;
			IMainFrameModule& mainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& mainFrameParentWindow = mainFrameModule.GetParentWindow();
			if ( mainFrameParentWindow.IsValid() && mainFrameParentWindow->GetNativeWindow().IsValid() )
			{
				parentWindowWindowHandle = mainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			//query for files
			const FString title = LOCTEXT("OpenAssetTitle", "Select FaceFX Asset").ToString();

			TArray<FString> files;
			desktopPlatform->OpenFileDialog(parentWindowWindowHandle, title, TEXT(""), TEXT(""), TEXT("FaceFX Asset (*.facefx)|*.facefx;"), EFileDialogFlags::None, files);

			if(files.Num() <= 0)
			{
				//no file selected
				ShowError(LOCTEXT("CreateFail", "Import of FaceFX asset cancelled."));
				return nullptr;
			}
			FaceFxAsset = files[0];
		}
		else
		{
			ShowError(LOCTEXT("CreateFail", "Internal Error: Retrieving platform module failed."));
			return nullptr;
		}
	}

	if(!FaceFxAsset.IsEmpty())
	{
		GWarn->BeginSlowTask(LOCTEXT("FaceFX_ImportProgress", "Importing FaceFX Assets..."), true);

		//create asset
		UFaceFxAsset* newAsset = ConstructObject<UFaceFxAsset>(Class, InParent, Name, Flags);

		//initialize asset
		FString errorMsg;
		const bool result = FFaceFxEditorTools::InitializeFromFile(newAsset, FaceFxAsset, &errorMsg, beforeDeletionCallback);
		GWarn->EndSlowTask();

		if(result)
		{
			//success
			FFaceFxEditorTools::SavePackage(newAsset->GetOutermost());
			ShowInfo(FText::FromString(FString::Printf(*LOCTEXT("CreateSuccess", "Import of FaceFX asset succeeded. (%i animations)").ToString(), newAsset->GetAnimationCount())));
			return newAsset;
		}
		else
		{
			//initialization failed
			ShowError(FText::FromString(FString::Printf(*LOCTEXT("InitSuccess", "Initialization of FaceFX asset failed. %s").ToString(), *errorMsg)));
		}
	}
	else
	{
		ShowError(LOCTEXT("CreateFail", "Import failed. FaceFX asset missing."));
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE