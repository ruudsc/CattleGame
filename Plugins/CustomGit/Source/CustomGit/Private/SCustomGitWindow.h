#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "CustomGitOperations.h"

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

    TSharedRef<FGitFileStatus> AsShared()
    {
        return MakeShareable(new FGitFileStatus(*this));
    }
};

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

    void Construct(const FArguments& InArgs);

    // Refresh the UI
    void RefreshStatus();

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

    // Update methods for data refresh
    void UpdateBranchList();
    void UpdateUserInfo();
    void UpdateCommitHistory();
    void UpdateCommandHistory();
    void UpdateStashList();

    // Callbacks from components
    void OnViewModeChanged(EGitViewMode NewMode);
    void OnCommitRequested(const FString& CommitMessage);
    void OnSyncClicked();
    void OnResetClicked();

    // File operations
    void OnStageFiles(const TArray<FString>& Files);
    void OnUnstageFiles(const TArray<FString>& Files);
    void OnLockFiles(const TArray<FString>& Files);
    void OnUnlockFiles(const TArray<FString>& Files);
    void OnDiscardFiles(const TArray<FString>& Files);
    void OnStashFiles(const TArray<FString>& Files);

    // Branch operations
    void OnSwitchBranch(const FString& BranchName);
    void OnCreateBranch(const FString& BranchName);
    void OnDeleteBranch(const FString& BranchName);
    void OnPushBranch(const FString& BranchName);
    void OnResetBranch(const FString& BranchName);

    // Stash operations
    void OnPopStash(const FString& StashRef);
    void OnDropStash(const FString& StashRef);

    // Sub-components
    TSharedPtr<class SCustomGitTopActionsBar> TopActionsBar;
    TSharedPtr<class SCustomGitViewModeSelector> ViewModeSelector;
    TSharedPtr<class SCustomGitFileListPanel> FileListPanel;
    TSharedPtr<class SCustomGitBranchPanel> BranchPanel;
    TSharedPtr<class SCustomGitSidebarTabs> SidebarTabs;
    TSharedPtr<class SCustomGitCommitPanel> CommitPanel;
};
