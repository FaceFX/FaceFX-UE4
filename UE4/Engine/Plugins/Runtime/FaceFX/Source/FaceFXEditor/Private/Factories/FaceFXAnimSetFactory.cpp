#include "FaceFXEditor.h"
#include "Factories/FaceFXAnimSetFactory.h"
#include "Factories/FaceFXActorFactory.h"
#include "FaceFXAnimSet.h"

UFaceFXAnimSetFactory::UFaceFXAnimSetFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP), bOnlyCreate(false)
{
	SupportedClass = UFaceFXAnimSet::StaticClass();
	bCreateNew = true;
	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("facefx;FaceFX Asset"));
}

UObject* UFaceFXAnimSetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if(bOnlyCreate)
	{
		return ConstructObject<UFaceFXAsset>(Class, InParent, Name, Flags);
	}
	return UFaceFXActorFactory::CreateNew(Class, InParent, Name, Flags);
}