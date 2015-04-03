#pragma once

/** The callback for when assets get imported and we want to do something before the compiled data gets deleted */
DECLARE_DELEGATE_ThreeParams(FCompilationBeforeDeletionDelegate, class UObject* /** asset */, const FString& /* compilationFolder */, bool /** loadResult */)