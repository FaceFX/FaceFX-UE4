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

#include "Include/Slate/FaceFXStyle.h"
#include "AssetTypeActions/AssetTypeActions_FaceFXActor.h"
#include "AssetTypeActions/AssetTypeActions_FaceFXAnim.h"
#include "FaceFXAnim.h"
#include "FaceFXCharacter.h"
#include "FaceFXEditorTools.h"

#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "FaceFX"

class FFaceFXEditorModule : public FDefaultModuleImpl
{
	virtual void StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		
		FFaceFXEditorTools::AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("FaceFX")), LOCTEXT("AssetCategory", "FaceFX"));
		
		AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFXActor));
		AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFXAnim));

		for(auto& Action : AssetTypeActions)
		{
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
		}

		FFaceFXStyle::Initialize();

		if(GIsEditor && FFaceFXEditorTools::IsShowToasterMessageOnIncompatibleAnim())
		{
			OnFaceFXCharacterPlayAssetIncompatibleHandle = UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible.AddRaw(this, &FFaceFXEditorModule::OnFaceFXCharacterPlayAssetIncompatible);
		}
	}

	virtual void ShutdownModule() override
	{
		if(OnFaceFXCharacterPlayAssetIncompatibleHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("FaceFX"))
		{
			UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible.Remove(OnFaceFXCharacterPlayAssetIncompatibleHandle);
		}

		if(FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools &AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

			for(auto& Action : AssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
				Action.Reset();
			}
		}

		FFaceFXStyle::Shutdown();
	}

private:

	/**
	* Events handler for when an asset was tried to be played on a FaceFX character instance which was incompatible to the actor handle
	* @param Character The FaceFX character instance who failed to play the animation
	* @param Asset The asset that was tried to be played and which was incompatible with the actor handle
	*/
	void OnFaceFXCharacterPlayAssetIncompatible(UFaceFXCharacter* Character, const UFaceFXAnim* Asset)
	{
		checkf(GIsEditor, TEXT("Must only be called from within the editor"));

		const AActor* CharacterOwner = Character ? Character->GetTypedOuter<AActor>() : nullptr;

		const FText Msg = FText::Format(LOCTEXT("OnFaceFXCharacterPlayAssetIncompatible", "FaceFX Animation \"{0}\" incompatible with FaceFX instance of actor {1}"), 
			Asset ? FText::FromString(Asset->GetName().ToString()) : FText::GetEmpty(), FText::FromString(CharacterOwner ? CharacterOwner->GetName() : GetNameSafe(Character)));

		FNotificationInfo Info(Msg);
		Info.ExpireDuration = 10.F;
		Info.bUseLargeFont = false;
		Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	FDelegateHandle OnFaceFXCharacterPlayAssetIncompatibleHandle;

	/** List of currently registered asset type actions */
	TArray<TSharedPtr<FAssetTypeActions_Base>> AssetTypeActions;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaceFXEditorModule, FaceFXEditor);