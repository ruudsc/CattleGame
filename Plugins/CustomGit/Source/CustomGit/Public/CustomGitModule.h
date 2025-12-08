#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCustomGitModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Gets the single instance of the CustomGit source control provider */
    class FCustomGitSourceControlProvider &GetProvider();

    /** Gets the singleton module instance */
    static FCustomGitModule &Get();

private:
    void RegisterMenus();
    TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs &SpawnTabArgs);

    /** Called before an asset is saved - attempt to lock LFS files */
    void OnPreAssetSave(UObject *Asset, FObjectPreSaveContext SaveContext);

private:
    /** The provider instance */
    TSharedPtr<class FCustomGitSourceControlProvider, ESPMode::ThreadSafe> GitProvider;

    /** Delegate handle for asset save callback */
    FDelegateHandle PreSaveHandle;
};
