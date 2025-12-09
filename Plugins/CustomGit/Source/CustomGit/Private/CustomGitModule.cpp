#include "CustomGitModule.h"
#include "CustomGitSourceControlProvider.h"
#include "Features/IModularFeatures.h"
#include "SCustomGitWindow.h"
#include "SLockFileDialog.h"
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

    // Check if this is a binary/LFS tracked file
    if (!FCustomGitOperations::IsBinaryAsset(PackageFilename))
    {
        return; // Not a binary file, no need to lock
    }

    // Clear the decision cache periodically (every new frame) to allow re-prompting
    // for files that were saved in a previous operation
    uint64 CurrentFrame = GFrameCounter;
    if (CurrentFrame != LastDecisionClearFrame)
    {
        FilesWithLockDecision.Empty();
        LastDecisionClearFrame = CurrentFrame;
    }

    // Check if user already made a decision for this file in this save operation
    if (FilesWithLockDecision.Contains(PackageFilename))
    {
        // User already decided, don't show dialog again
        return;
    }

    // Get locks with proper ownership detection using --verify
    TSet<FString> OurLocks;
    TMap<FString, FString> OtherLocks;
    FCustomGitOperations::GetLocksWithOwnership(OurLocks, OtherLocks);

    FString RelativePath = PackageFilename;
    FString RepoRoot = FCustomGitOperations::GetRepositoryRoot();
    FPaths::MakePathRelativeTo(RelativePath, *(RepoRoot + TEXT("/")));
    RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));
    FString Filename = FPaths::GetCleanFilename(PackageFilename);

    // Check if we own the lock (check multiple path formats)
    bool bIsLockedByMe = OurLocks.Contains(RelativePath);
    if (!bIsLockedByMe)
    {
        for (const FString &OurLock : OurLocks)
        {
            if (OurLock.EndsWith(Filename) || FPaths::GetCleanFilename(OurLock) == Filename)
            {
                bIsLockedByMe = true;
                break;
            }
        }
    }

    // If locked by us, just proceed with the save - no dialog needed
    if (bIsLockedByMe)
    {
        UE_LOG(LogTemp, Log, TEXT("CustomGit: File already locked by us: %s"), *PackageFilename);
        return;
    }

    // Check if someone else has it locked
    bool bIsLockedByOther = false;
    FString LockOwner;

    if (const FString *Owner = OtherLocks.Find(RelativePath))
    {
        bIsLockedByOther = true;
        LockOwner = *Owner;
    }
    else
    {
        for (const auto &LockPair : OtherLocks)
        {
            if (LockPair.Key.EndsWith(Filename) || FPaths::GetCleanFilename(LockPair.Key) == Filename)
            {
                bIsLockedByOther = true;
                LockOwner = LockPair.Value;
                break;
            }
        }
    }

    // Show the lock dialog
    FString DisplayFilename = FPaths::GetCleanFilename(PackageFilename);
    ELockFileDialogResult DialogResult;

    if (bIsLockedByOther)
    {
        // Locked by someone else - only show Save Without Locking and Cancel
        DialogResult = SLockFileDialog::ShowDialog(
            DisplayFilename,
            FString(), // No error message
            true,      // Is locked by other
            LockOwner);
    }
    else
    {
        // File is not locked - ask if user wants to lock it
        DialogResult = SLockFileDialog::ShowDialog(DisplayFilename);
    }

    // Handle the user's choice
    switch (DialogResult)
    {
    case ELockFileDialogResult::Lock:
    {
        // This should only be called when file is NOT locked by someone else
        FString Error;
        bool bLockSuccess = FCustomGitOperations::LockFile(PackageFilename, Error);

        if (bLockSuccess)
        {
            // Make file writable
            FCustomGitOperations::SetFileReadOnly(PackageFilename, false);
            UE_LOG(LogTemp, Log, TEXT("CustomGit: Successfully locked file: %s"), *PackageFilename);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CustomGit: Failed to lock file %s: %s"), *PackageFilename, *Error);
        }
        // Mark decision so we don't prompt again
        FilesWithLockDecision.Add(PackageFilename);
        break;
    }

    case ELockFileDialogResult::SaveWithoutLock:
    {
        // User chose to save without locking - proceed but log a warning
        // Mark that user made a decision so we don't prompt again
        FilesWithLockDecision.Add(PackageFilename);
        UE_LOG(LogTemp, Warning, TEXT("CustomGit: Saving without lock: %s"), *PackageFilename);
        break;
    }

    case ELockFileDialogResult::Cancel:
    {
        // User cancelled - mark the decision so we don't show dialog again
        // Note: We can't actually cancel the save from this callback
        FilesWithLockDecision.Add(PackageFilename);
        UE_LOG(LogTemp, Log, TEXT("CustomGit: User cancelled save for: %s (save may still proceed)"), *PackageFilename);
        break;
    }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCustomGitModule, CustomGit)
