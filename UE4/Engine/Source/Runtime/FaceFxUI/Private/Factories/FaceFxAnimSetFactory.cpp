#include "FaceFxUI.h"

#include "Factories/FaceFxAnimSetFactory.h"
#if WITH_FACEFX
#include "Factories/FaceFxActorFactory.h"
#include "FaceFxAnimSet.h"
#endif

UFaceFxAnimSetFactory::UFaceFxAnimSetFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP), m_onlyCreate(false)
{
#if WITH_FACEFX
	SupportedClass = UFaceFxAnimSet::StaticClass();
	bCreateNew = true;
	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("facefx;FaceFX Asset"));
#endif
}

UObject* UFaceFxAnimSetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
#if WITH_FACEFX
	if(m_onlyCreate)
	{
		return ConstructObject<UFaceFxAsset>(Class, InParent, Name, Flags);
	}
	return UFaceFxActorFactory::CreateNew(Class, InParent, Name, Flags);
#else
	return nullptr;
#endif
}