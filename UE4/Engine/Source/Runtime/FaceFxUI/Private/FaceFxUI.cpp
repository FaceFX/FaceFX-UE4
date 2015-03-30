#include "FaceFxUI.h"

#include "AssetTypeActions/AssetTypeActions_FaceFxActor.h"
#include "AssetTypeActions/AssetTypeActions_FaceFxAnimSet.h"

class FFaceFxUIModule : public FDefaultModuleImpl
{
	virtual void StartupModule() override
	{
		IAssetTools &assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		m_assetTypeActions.Add( MakeShareable( new FAssetTypeActions_FaceFxActor) );
		m_assetTypeActions.Add( MakeShareable( new FAssetTypeActions_FaceFxAnimSet) );

		for(auto& action : m_assetTypeActions)
		{
			assetTools.RegisterAssetTypeActions(action.ToSharedRef());
		}
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
	}

private:

	TArray< TSharedPtr< FAssetTypeActions_Base > > m_assetTypeActions;
};

IMPLEMENT_MODULE( FFaceFxUIModule, FaceFxUI );