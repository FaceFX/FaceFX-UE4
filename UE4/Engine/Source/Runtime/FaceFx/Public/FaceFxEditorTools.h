#if WITH_EDITOR

/** The target platform for FaceFX platform specific compilation data */
namespace EFaceFxTargetPlatform
{
	enum Type
	{
		PC = 0,
		PS4,
		XBoxOne,
		MAX
	};
}

/** Editor specific FaceFX functions */
struct FACEFX_API FFaceFxEditorTools
{

	/**
	* Gets the path to the FaceFX Studio installation. Configurable via engine ini file (section "ThirdParty.FaceFX", property "StudioPathAbsolute")
	* @returns The path to the installation
	*/
	static const FString& GetFaceFXStudioPath();

	/**
	* Gets the path to the FaceFX compiler. Configurable via engine ini file (section "ThirdParty.FaceFX", property "CompilerPathRelative")
	* @returns The path to the FaceFX compiler
	*/
	static const FString& GetFaceFXCompilerPath();

	/**
	* Checks if FaceFX studio is installed within the standard installation path
	* @returns True if installed, else false
	*/
	static bool IsFaceFXStudioInstalled();

	/**
	* Gets the indicator if the compiler is installed
	* @returns True if installed else false
	*/
	static bool IsFaceFXCompilerInstalled();

	/**
	* Opens the given FaceFX asset within FaceFX studio in case this is installed locally
	* @param asset The asset to open
	* @param outErrorMessage The optional error message in case it failed
	* @returns True if open succeeded, else false
	*/
	static bool OpenFaceFXStudio(UFaceFxActor* asset, FString* outErrorMessage = nullptr);

	/**
	* Reimports a given FaceFX asset
	* @param target The target asset to reimport
	* @param outErrorMessage The optional error message in case it failed
	* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ReImportFaceFXAsset(UFaceFxAsset* target, FString* outErrorMessage = nullptr, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Imports a given FaceFX asset
	* @param target The target asset to import into
	* @param assetPath The path to the .facefx asset file
	* @param outErrorMessage The optional error message in case it failed
	* @param beforeDeletionCallback Callback that gets called before the compilation folder gets deleted again
	* @returns True if open succeeded, else false
	*/
	static bool ImportFaceFXAsset(UFaceFxAsset* target, const FString& assetPath, FString* outErrorMessage = nullptr, const FCompilationBeforeDeletionDelegate& beforeDeletionCallback = FCompilationBeforeDeletionDelegate());

	/**
	* Compiles the given .facefx asset file within a new temporary folder
	* @param assetPath The file path
	* @param outCompiledFile The temporary file which will be in the temporary folder that also contain all the compiled assets
	* @param outErrorMessage The optional error message in case it failed
	* @returns True if open succeeded, else false
	*/
	static bool CompileAssetToTempFolder(const FString& assetPath, FString& outCompiledFile, FString* outErrorMessage = nullptr);

	/** 
	* Loads all assets from a given folder into the given target struct
	* @param folder The path to the folder to load the compiled assets from
	* @param assetName The name of the assets to load without any extension
	* @param targetData The target struct to load into
	* @returns True if succeeded, else false
	*/
	static bool LoadCompiledData(const FString& folder, const FString& assetName, FFaceFxActorData& targetData);

	/** 
	* Loads all assets from a given folder into the given target struct
	* @param folder The path to the folder to load the compiled assets from
	* @param targetData The target struct to load into
	* @returns True if succeeded, else false
	*/
	static bool LoadCompiledData(const FString& folder, FFaceFxAnimSetData& targetData);

	/**
	* Gets all animation groups found within a given FaceFx compilation folder and platform specific sub folder
	* @param folder The compilation folder to load from
	* @param platform The target platform to use the sub folder from
	* @param outGroups The resulting animation groups
	* @returns True if succeeded, else false
	*/
	inline static bool GetAnimationGroupsInFolder(const FString& folder, EFaceFxTargetPlatform::Type platform, TArray<FString>& outGroups)
	{
		return GetAnimationGroupsInFolder(GetCompilationFolder(folder, platform), outGroups);
	}

	/**
	* Gets all animation groups found within a given FaceFx compilation folder
	* @param folder The compilation folder to load from
	* @param outGroups The resulting animation groups
	* @returns True if succeeded, else false
	*/
	static bool GetAnimationGroupsInFolder(const FString& folder, TArray<FString>& outGroups);

	/**
	* Gets the FaceFX temp folder
	* @returns The temp folder
	*/
	inline static FString GetTempFolder()
	{
		return FPaths::GameSavedDir() / TEXT("TmpFaceFx");
	}

	/**
	* Gets the compilation sub folder for a given folder
	* @param folder The base folder to generate the target folder for
	* @param platform The target platform
	* @returns The target folder
	*/
	static inline FString GetCompilationFolder(const FString& folder, EFaceFxTargetPlatform::Type platform)
	{
		switch(platform)
		{
		case EFaceFxTargetPlatform::PC: return folder / TEXT("x86");
		case EFaceFxTargetPlatform::PS4: return folder / TEXT("ps4");
		case EFaceFxTargetPlatform::XBoxOne: return folder / TEXT("xboxone");
		default: checkf(false, TEXT("Unknown target platform type"));
		}
		return TEXT("");
	}

	/**
	* Gets the timeout duration set in seconds. Default is 120sec. Configurable via engine ini file (section "Yager.ThirdParty.FaceFX", property "CompilationTimeoutSec")
	* @returns The current duration
	*/
	static float GetCompilationTimeoutDuration();

	/**
	* Creates a new anim set asset
	* @param compilationFolder The folder to load the data from
	* @param animGroup The animation group folder to load the data from
	* @param packageName The name of the package
	* @param fxActor The FaceFX actor asset that shall be linked to that new asset
	* @param assetTools The asset tools instance to use
	* @param factory The factory to use. Keep at nullptr to directly create an instance
	*/
	static class UFaceFxAnimSet* CreateAnimSetAsset(const FString& compilationFolder, const FString& animGroup, const FString& packageName, class UFaceFxActor* fxActor, class IAssetTools& assetTools, class UFactory* factory = nullptr);

	/**
	* Performs steps to store an asset physically and in source control
	* @param package The package to save
	*/
	static void SavePackage(UPackage* package);

private:

	FFaceFxEditorTools(){}
};

#endif //WITH_EDITOR