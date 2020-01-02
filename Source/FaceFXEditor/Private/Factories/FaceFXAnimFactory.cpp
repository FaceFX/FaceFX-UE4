/*******************************************************************************
  The MIT License (MIT)
  Copyright (c) 2015-2020 OC3 Entertainment, Inc. All rights reserved.
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

#include "Factories/FaceFXAnimFactory.h"
#include "FaceFXEditor.h"
#include "Include/Slate/FaceFXStyle.h"
#include "Factories/FaceFXActorFactory.h"
#include "FaceFXAnim.h"

UFaceFXAnimFactory::UFaceFXAnimFactory(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	SupportedClass = UFaceFXAnim::StaticClass();
	bCreateNew = false;
	bEditorImport = false;
	bText = false;
}

uint32 UFaceFXAnimFactory::GetMenuCategories() const
{
	return FFaceFXEditorTools::AssetCategory;
}

UObject* UFaceFXAnimFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UFaceFXAsset>(InParent, Class, Name, Flags);
}

FName UFaceFXAnimFactory::GetNewAssetThumbnailOverride() const
{
	return FFaceFXStyle::GetBrushIdFxAnim();
}
