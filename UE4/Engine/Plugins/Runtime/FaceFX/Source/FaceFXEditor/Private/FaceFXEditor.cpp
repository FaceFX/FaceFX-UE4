#include "FaceFXEditor.h"

#include "Include/Slate/FaceFXStyle.h"
#include "AssetTypeActions/AssetTypeActions_FaceFXActor.h"
#include "AssetTypeActions/AssetTypeActions_FaceFXAnimSet.h"

class FFaceFXEditorModule : public FDefaultModuleImpl
{
	virtual void StartupModule() override
	{
		IAssetTools &AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFXActor));
		AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFXAnimSet));

		for(auto& Action : AssetTypeActions)
		{
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
		}

		FFaceFXStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
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

	/** List of currently registered asset type actions */
	TArray<TSharedPtr<FAssetTypeActions_Base>> AssetTypeActions;
};

IMPLEMENT_MODULE(FFaceFXEditorModule, FaceFXEditor);