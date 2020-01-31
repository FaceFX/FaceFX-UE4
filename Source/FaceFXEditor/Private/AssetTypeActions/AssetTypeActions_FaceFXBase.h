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

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_FaceFXBase : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FColor GetTypeColor() const override { return FColor(137,191,53); }
	virtual bool CanFilter() override { return true; }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual uint32 GetCategories() override;

protected:

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when SetSource is selected */
	void ExecuteSetSource(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when ShowDetails is selected */
	void ExecuteShowDetails(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can recompile assets */
	bool CanExecuteReimport(const TArray<UObject*> Objects) const;

	/** Determine if we can set a new source */
	bool CanExecuteSetSource(const TArray<UObject*> Objects) const;

	/** Determine if we can show details */
	bool CanExecuteShowDetails(const TArray<UObject*> Objects) const;

	/** Handler for when OpenFolder is selected */
	void ExecuteOpenFolder(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Determine if we can open the source asset folder */
	bool CanExecuteOpenFolder(const TArray<UObject*> Objects) const;

private:

	/** Callback function for before the compilation folder gets deleted */
	void OnReimportBeforeDelete(class UObject* Asset, const FString& CompilationFolder, bool LoadResult, struct FFaceFXImportResult& OutResultMessages);
};
