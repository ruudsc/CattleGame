#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "CustomGitOperations.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "TimerManager.h"

#include "SCustomGitViewModeSelector.h"
#include "SCustomGitFileListPanel.h"
#include "SCustomGitCommitPanel.h"
#include "SCustomGitBranchPanel.h"
#include "SCustomGitTopActionsBar.h"
#include "SCustomGitSidebarTabs.h"

// Simple struct to hold file status info for the list view
struct FGitFileStatus
{
    FString Filename;
    FString Status; // e.g. "M", "A", "??"
    FDateTime ModificationTime;
    bool bIsLockedByUs = false; // True if this file is locked by the current user

    TSharedRef<FGitFileStatus> AsShared()
    {
        return MakeShareable(new FGitFileStatus(*this));
    }
};

// Forward declaration
class SCustomGitWindow;

/**
 * FGitFileWatcher - Background thread for monitoring git repository changes
 * Watches .git/index and .git/refs for modifications to trigger UI refresh
 */
class FGitFileWatcher : public FRunnable
{
public:
    FGitFileWatcher(SCustomGitWindow* InParent);
    virtual ~FGitFileWatcher() {}

    // FRunnable interface
    virtual bool Init() override { return true; }
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override {}

private:
    SCustomGitWindow* Parent;
    FString GitIndexPath;
    FString GitRefsPath;
    FString GitHeadPath;
    FDateTime LastIndexModTime;
    FDateTime LastRefsModTime;
    FDateTime LastHeadModTime;
    TAtomic<bool> bKeepRunning;
};

// Enum for refresh types (bitflags)
enum class EGitRefreshType : uint8
{
    None = 0,
    Status = 1 << 0,       // File status changed
    Branches = 1 << 1,     // Branch refs changed
    Locks = 1 << 2,        // Lock status changed
    All = 0xFF
};
ENUM_CLASS_FLAGS(EGitRefreshType);

/**
 * SCustomGitWindow - Main git window coordinator
 *
 * This is the primary window for git operations. It owns all data arrays and coordinates
 * between the various sub-components:
 * - SCustomGitTopActionsBar - Sync/Reset buttons
 * - SCustomGitViewModeSelector - Local/Staged/Locked file view selector
 * - SCustomGitFileListPanel - File list with drag-drop and context menus
 * - SCustomGitBranchPanel - Branch list and operations
 * - SCustomGitSidebarTabs - Stashes, Commit History, Command History tabs
 * - SCustomGitCommitPanel - Commit message input
 */
class SCustomGitWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs);
    virtual ~SCustomGitWindow();

    // Refresh the UI
    void RefreshStatus();

    // Called by file watcher when git index changes (thread-safe)
    void OnGitIndexChanged();
    // Called by file watcher when git refs change (thread-safe)
    void OnGitRefsChanged();

private:
    // Data Source - owned by main window, shared with components
    TArray<TSharedPtr<FGitFileStatus>> LocalFileList;
    TArray<TSharedPtr<FGitFileStatus>> StagedFileList;
    TArray<TSharedPtr<FGitFileStatus>> LockedFileList;
    TArray<TSharedPtr<FGitFileStatus>> HistoryFileList;

    TArray<TSharedPtr<FGitBranchInfo>> BranchList;
    TArray<TSharedPtr<FString>> StashList;
    TArray<TSharedPtr<FString>> CommitHistoryList;
    TArray<TSharedPtr<FString>> CommandHistoryList;

    FString CurrentBranchName;
    FString CurrentUserName;
    FString CurrentUserEmail;
    EGitViewMode CurrentViewMode = EGitViewMode::LocalChanges;

    // Auto-refresh file watcher (Approach 2)
    TUniquePtr<FGitFileWatcher> FileWatcher;
    FRunnableThread* FileWatcherThread = nullptr;

    // Debounce timer for auto-refresh
    FTimerHandle DebounceTimerHandle;
    EGitRefreshType PendingRefreshType = EGitRefreshType::None;
    static constexpr float DEBOUNCE_DELAY = 0.5f; // Wait 500ms after last change

    // Debounced refresh methods
    void ScheduleDebouncedRefresh(EGitRefreshType RefreshType);
    void OnDebouncedRefresh();
    void RefreshStatusOnly();

    // Update methods for data refresh
    void UpdateBranchList();
    void UpdateUserInfo();
    void UpdateCommitHistory();
    void UpdateCommandHistory();
    void UpdateStashList();

    // Callbacks from components
    void OnViewModeChanged(EGitViewMode NewMode);
    void OnCommitRequested(const FString &CommitMessage);
    void OnSyncClicked();
    void OnResetClicked();

    // File operations
    void OnStageFiles(const TArray<FString> &Files);
    void OnUnstageFiles(const TArray<FString> &Files);
    void OnLockFiles(const TArray<FString> &Files);
    void OnUnlockFiles(const TArray<FString> &Files);
    void OnDiscardFiles(const TArray<FString> &Files);
    void OnStashFiles(const TArray<FString> &Files);

    // Branch operations
    void OnSwitchBranch(const FString &BranchName);
    void OnCreateBranch(const FString &BranchName);
    void OnDeleteBranch(const FString &BranchName);
    void OnPushBranch(const FString &BranchName);
    void OnResetBranch(const FString &BranchName);

    // Stash operations
    void OnPopStash(const FString &StashRef);
    void OnDropStash(const FString &StashRef);

    // Sub-components
    TSharedPtr<class SCustomGitTopActionsBar> TopActionsBar;
    TSharedPtr<class SCustomGitViewModeSelector> ViewModeSelector;
    TSharedPtr<class SCustomGitFileListPanel> FileListPanel;
    TSharedPtr<class SCustomGitBranchPanel> BranchPanel;
    TSharedPtr<class SCustomGitSidebarTabs> SidebarTabs;
    TSharedPtr<class SCustomGitCommitPanel> CommitPanel;
};
