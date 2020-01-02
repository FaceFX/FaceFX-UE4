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

#pragma once

#include "FaceFXConfig.h"
#include "AssetTypeActions_FaceFXBase.h"

class FAssetTypeActions_FaceFXActor : public FAssetTypeActions_FaceFXBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor ) override;

private:

	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can edit assets */
	bool CanExecuteEdit(const TArray<UObject*> Objects) const;

#if FACEFX_USEANIMATIONLINKAGE

	/** Handler for when Link is selected */
	void ExecuteLink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when Unlink is selected */
	void ExecuteUnlink(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Callback for when the assets to link with have been chosen for a set of objects */
	static void OnAssetLinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects);

	/** Callback for when the assets to unlink with have been chosen for a set of objects */
	static void OnAssetUnlinkChosen(const TArray<FAssetData>& SelectedAssets, TArray<TWeakObjectPtr<UObject>> SelectedObjects);

#endif //FACEFX_USEANIMATIONLINKAGE
};
