#include "CustomGitModule.h"
#include "CustomGitSourceControlProvider.h"
#include "Features/IModularFeatures.h"
#include "SCustomGitWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "CustomGitOperations.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"

static const FName CustomGitTabName("CustomGitWindow");

#define LOCTEXT_NAMESPACE "FCustomGitModule"

void FCustomGitModule::StartupModule()
{
    // Create the provider
    GitProvider = MakeShareable(new FCustomGitSourceControlProvider);

    // Register module features
    IModularFeatures::Get().RegisterModularFeature("SourceControl", GitProvider.Get());

    // Register Tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CustomGitTabName, FOnSpawnTab::CreateRaw(this, &FCustomGitModule::OnSpawnPluginTab)).SetDisplayName(LOCTEXT("CustomGitTabTitle", "Git Controls")).SetMenuType(ETabSpawnerMenuType::Hidden); // We register manually in menus if needed, or allow it to be found in Window -> Git Controls if we use proper Group.
    // Actually, let's put it in Window menu via ToolMenus

    RegisterMenus();

    // Register for asset save events to auto-lock LFS files
    PreSaveHandle = FCoreUObjectDelegates::OnObjectPreSave.AddRaw(this, &FCustomGitModule::OnPreAssetSave);
}

void FCustomGitModule::ShutdownModule()
{
    // Unregister asset save callback
    if (PreSaveHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectPreSave.Remove(PreSaveHandle);
    }

    // Unregister module features
    IModularFeatures::Get().UnregisterModularFeature("SourceControl", GitProvider.Get());

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CustomGitTabName);

    // Clean up
    if (GitProvider.IsValid())
    {
        GitProvider->Close();
        GitProvider = nullptr;
    }
}

FCustomGitSourceControlProvider &FCustomGitModule::GetProvider()
{
    return *GitProvider.Get();
}

FCustomGitModule &FCustomGitModule::Get()
{
    return FModuleManager::GetModuleChecked<FCustomGitModule>("CustomGit");
}

void FCustomGitModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu *Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
        FToolMenuSection &Section = Menu->FindOrAddSection("WindowLayout");
        Section.AddMenuEntry(
            "GitControls",
            LOCTEXT("GitControls", "Git Controls"),
            LOCTEXT("GitControlsTooltip", "Open Git Controls Window"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([]()
                                             { FGlobalTabmanager::Get()->TryInvokeTab(CustomGitTabName); })));
    }

    // Extend Source Control Menu (Bottom Right status bar)
    // Try the StatusBar.SourceControl menu
    UToolMenu *SCMenu = UToolMenus::Get()->ExtendMenu("StatusBar.ToolBar.SourceControl");
    if (SCMenu)
    {
        FToolMenuSection &Section = SCMenu->FindOrAddSection("SourceControlActions");
        Section.AddMenuEntry(
            "OpenGitControls",
            LOCTEXT("OpenGitControls", "Open Git Controls"),
            LOCTEXT("OpenGitControlsTooltip", "Open the Git Controls Window"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Submit"),
            FUIAction(
                FExecuteAction::CreateLambda([]()
                                             { FGlobalTabmanager::Get()->TryInvokeTab(CustomGitTabName); })));
    }

    // Also try the generic SourceControl menu
    UToolMenu *SCMenu2 = UToolMenus::Get()->ExtendMenu("SourceControl.Menu");
    if (SCMenu2)
    {
        FToolMenuSection &Section = SCMenu2->FindOrAddSection("SourceControlActions");
        Section.AddMenuEntry(
            "OpenGitControls2",
            LOCTEXT("OpenGitControls2", "Open Git Controls"),
            LOCTEXT("OpenGitControlsTooltip2", "Open the Git Controls Window"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Submit"),
            FUIAction(
                FExecuteAction::CreateLambda([]()
                                             { FGlobalTabmanager::Get()->TryInvokeTab(CustomGitTabName); })));
    }
}

TSharedRef<SDockTab> FCustomGitModule::OnSpawnPluginTab(const FSpawnTabArgs &SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
            [SNew(SCustomGitWindow)];
}

void FCustomGitModule::OnPreAssetSave(UObject *Asset, FObjectPreSaveContext SaveContext)
{
    if (!Asset)
    {
        return;
    }

    // Get the package/file path
    UPackage *Package = Asset->GetOutermost();
    if (!Package)
    {
        return;
    }

    FString PackageFilename;
    if (!FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFilename, Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension()))
    {
        return;
    }

    // Check if this is an LFS tracked file and try to lock it
    if (FCustomGitOperations::IsLFSTrackedFile(PackageFilename))
    {
        FString Error;
        if (!FCustomGitOperations::LockFileIfLFS(PackageFilename, Error, false))
        {
            // Show warning dialog
            FText Title = LOCTEXT("LFSLockWarningTitle", "Git LFS Lock Warning");
            FText Message = FText::Format(
                LOCTEXT("LFSLockWarningMessage",
                        "Warning: Could not acquire LFS lock for binary asset:\n\n{0}\n\n{1}\n\n"
                        "This file may be locked by another user. Saving without a lock may cause merge conflicts.\n\n"
                        "Do you want to continue saving anyway?"),
                FText::FromString(FPaths::GetCleanFilename(PackageFilename)),
                FText::FromString(Error));

            EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message, Title);

            if (Result == EAppReturnType::No)
            {
                // User chose not to save - we can't cancel the save from here directly,
                // but we've warned them. In a more complete implementation, we'd use
                // CanModify delegates or the source control checkout flow.
                UE_LOG(LogTemp, Warning, TEXT("CustomGit: User was warned about LFS lock failure for %s"), *PackageFilename);
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("CustomGit: Successfully locked LFS file %s"), *PackageFilename);
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCustomGitModule, CustomGit)
