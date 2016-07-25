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
#include "Sequencer/FaceFXSequencer.h"
#include "Sequencer/FaceFXAnimationTrackEditor.h"

#include "Matinee/MatineeActor.h"

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
		FFaceFXSequencer::Get().Initialize();

		if(GIsEditor && FFaceFXEditorTools::IsShowToasterMessageOnIncompatibleAnim())
		{
			OnFaceFXCharacterPlayAssetIncompatibleHandle = UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible.AddRaw(this, &FFaceFXEditorModule::OnFaceFXCharacterPlayAssetIncompatible);
		}

		OnEndPieHandle = FEditorDelegates::EndPIE.AddStatic(&FFaceFXEditorModule::OnEndPie);
		OnPreSaveWorldHandle = FEditorDelegates::PreSaveWorld.AddStatic(&FFaceFXEditorModule::PreSaveWorld);
	}

	virtual void ShutdownModule() override
	{
		if(OnFaceFXCharacterPlayAssetIncompatibleHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("FaceFX"))
		{
			UFaceFXCharacter::OnFaceFXCharacterPlayAssetIncompatible.Remove(OnFaceFXCharacterPlayAssetIncompatibleHandle);
		}

		FEditorDelegates::EndPIE.Remove(OnEndPieHandle);
		FEditorDelegates::PreSaveWorld.Remove(OnPreSaveWorldHandle);
		
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
		FFaceFXSequencer::Get().Shutdown();
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

		/** Helper struct to prevent spamming of error message coming from either matinee or sequencer */
		struct FFaceFXErrorMessageShownData
		{
			/** The editor instance currently assigned */
			void* Editor;

			/** The error messages displayed for the currently assigned editor instance */
			TArray<TPair<UFaceFXCharacter*, const UFaceFXAnim*>> ShownErrors;

			FFaceFXErrorMessageShownData() : Editor(nullptr){}

      // Note: jcr -- renamed Character & Asset to Character_ & Asset_ so as not to shadow Character & Asset parameters to OnFaceFXCharacterPlayAssetIncompatible
			/** 
			* Updates the data
			* @param SourceEditor The editor which updates this data
			* @param Character_ The FaceFX character instance who failed to play the animation
			* @param Asset_ The asset that was tried to be played and which was incompatible with the actor handle
			* @returns True if the error message shall be displayed, false if it was already shown
			*/
			inline bool Update(void* SourceEditor, UFaceFXCharacter* Character_, const UFaceFXAnim* Asset_)
			{
				if (SourceEditor && Editor == SourceEditor)
				{
					//while being within editor we only show the error once
					TPairInitializer<UFaceFXCharacter*, const UFaceFXAnim*> Entry(Character_, Asset_);
					if (ShownErrors.Contains(Entry))
					{
						//already shown once
						return false;
					}
					ShownErrors.Add(Entry);
				}
				else
				{
					ShownErrors.Reset();
				}
				Editor = SourceEditor;
				return SourceEditor != nullptr;
			}
		};

		//prevent error message spamming of matinee and sequencer editors
		static FFaceFXErrorMessageShownData MatineeErrorData;
		static FFaceFXErrorMessageShownData SequencerErrorData;
		const bool ShowMessageMatinee = MatineeErrorData.Update(GEditor->ActiveMatinee.Get(), Character, Asset);
		const bool ShowMessageSequencer = SequencerErrorData.Update(FFaceFXAnimationTrackEditor::GetCurrentSequencer().Pin().Get(), Character, Asset);
		
		if (!ShowMessageMatinee && !ShowMessageSequencer)
		{
			return;
		}

		//prepare and show error message
		const AActor* CharacterOwner = Character ? Character->GetTypedOuter<AActor>() : nullptr;

		const FText Msg = FText::Format(LOCTEXT("OnFaceFXCharacterPlayAssetIncompatible", "FaceFX Animation \"{0}\" incompatible with FaceFX instance of actor {1}"), 
			Asset ? FText::FromString(Asset->GetName().ToString()) : FText::GetEmpty(), FText::FromString(CharacterOwner ? CharacterOwner->GetName() : GetNameSafe(Character)));

		FFaceFXEditorTools::ShowError(Msg);
	}

	FDelegateHandle OnFaceFXCharacterPlayAssetIncompatibleHandle;

	/** The event callback handle for OnEndPie */
	FDelegateHandle OnEndPieHandle;

	/** The event callback handle for OnPreSaveWorldHandle */
	FDelegateHandle OnPreSaveWorldHandle;

	/** Callback for when an PIE got ended */
	static void OnEndPie(bool bIsSimulating)
	{
		TArray<UObject*> CharacterInstances;
		GetObjectsOfClass(UFaceFXCharacter::StaticClass(), CharacterInstances);

		for (UObject* CharacterInstance : CharacterInstances)
		{
			CastChecked<UFaceFXCharacter>(CharacterInstance)->Stop(true);
		}
	}

	/** Callback for when a world gets saved */
	static void PreSaveWorld(uint32 SaveFlags, UWorld* World)
	{
		check(World);
		
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = (*It);
			check(Actor);
			const TArray<UActorComponent*> CharacterInstances = Actor->GetComponentsByClass(UFaceFXCharacter::StaticClass());
			for (UActorComponent* CharacterInstance : CharacterInstances)
			{
				CastChecked<UFaceFXCharacter>(CharacterInstance)->Stop(true);
			}
		}
	}

	/** List of currently registered asset type actions */
	TArray<TSharedPtr<FAssetTypeActions_Base>> AssetTypeActions;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaceFXEditorModule, FaceFXEditor);
