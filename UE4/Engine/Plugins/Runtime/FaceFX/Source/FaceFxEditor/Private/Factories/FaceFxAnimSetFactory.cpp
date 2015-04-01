#include "FaceFxEditor.h"

#include "Factories/FaceFxAnimSetFactory.h"
#include "Factories/FaceFxActorFactory.h"
#include "FaceFxAnimSet.h"

UFaceFxAnimSetFactory::UFaceFxAnimSetFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP), m_onlyCreate(false)
{
	SupportedClass = UFaceFxAnimSet::StaticClass();
	bCreateNew = true;
	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("facefx;FaceFX Asset"));
}

UObject* UFaceFxAnimSetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if(m_onlyCreate)
	{
		return ConstructObject<UFaceFxAsset>(Class, InParent, Name, Flags);
	}
	return UFaceFxActorFactory::CreateNew(Class, InParent, Name, Flags);
}