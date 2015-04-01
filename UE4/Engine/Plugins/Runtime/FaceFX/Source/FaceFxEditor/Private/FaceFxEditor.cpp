#include "FaceFxEditor.h"

#include "Include/Slate/FaceFxStyle.h"
#include "AssetTypeActions/AssetTypeActions_FaceFxActor.h"
#include "AssetTypeActions/AssetTypeActions_FaceFxAnimSet.h"

class FFaceFxEditorModule : public FDefaultModuleImpl
{
	virtual void StartupModule() override
	{
		IAssetTools &assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		m_assetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFxActor));
		m_assetTypeActions.Add(MakeShareable(new FAssetTypeActions_FaceFxAnimSet));

		for(auto& action : m_assetTypeActions)
		{
			assetTools.RegisterAssetTypeActions(action.ToSharedRef());
		}

		FFaceFxStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		if(FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools &assetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

			for(auto& action : m_assetTypeActions)
			{
				assetTools.UnregisterAssetTypeActions(action.ToSharedRef());
				action.Reset();
			}
		}

		FFaceFxStyle::Shutdown();
	}

private:

	/** List of currently registered asset type actions */
	TArray<TSharedPtr<FAssetTypeActions_Base>> m_assetTypeActions;
};

IMPLEMENT_MODULE(FFaceFxEditorModule, FaceFxEditor);