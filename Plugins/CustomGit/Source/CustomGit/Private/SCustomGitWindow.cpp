#include "SCustomGitWindow.h"
#include "CustomGitOperations.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "SCustomGitWindow"

void SCustomGitWindow::Construct(const FArguments& InArgs)
{
    CurrentViewMode = EGitViewMode::LocalChanges;

    // Build UI
    ChildSlot
        [SNew(SSplitter)
            .Orientation(Orient_Horizontal)

            // --- Sidebar (Left) ---
            + SSplitter::Slot()
                .Value(0.25f)
                [SNew(SBorder)
                    .Padding(5.0f)
                    .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    [SNew(SVerticalBox)

                        // Top actions bar
                        + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(2)
                            [SAssignNew(TopActionsBar, SCustomGitTopActionsBar)
                                .CurrentBranchName(CurrentBranchName)
                                .OnSyncClicked_Lambda([this]()
                                {
                                    OnSyncClicked();
                                })
                                .OnResetClicked_Lambda([this]()
                                {
                                    OnResetClicked();
                                })]

                        + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 5, 0, 5)
                            [SNew(SSeparator)
                                .Orientation(Orient_Horizontal)]

                        // View mode selector
                        + SVerticalBox::Slot()
                            .AutoHeight()
                            [SAssignNew(ViewModeSelector, SCustomGitViewModeSelector)
                                .InitialViewMode(EGitViewMode::LocalChanges)
                                .OnViewModeChanged_Lambda([this](EGitViewMode NewMode)
                                {
                                    OnViewModeChanged(NewMode);
                                })]

                        + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 10, 0, 5)
                            [SNew(SSeparator)
                                .Orientation(Orient_Horizontal)]

                        // Branch panel
                        + SVerticalBox::Slot()
                            .FillHeight(1.0f)
                            [SAssignNew(BranchPanel, SCustomGitBranchPanel)
                                .BranchList(&BranchList)
                                .CurrentBranchName(CurrentBranchName)
                                .OnSwitchBranch_Lambda([this](const FString& BranchName)
                                {
                                    OnSwitchBranch(BranchName);
                                })
                                .OnCreateBranch_Lambda([this](const FString& BranchName)
                                {
                                    OnCreateBranch(BranchName);
                                })
                                .OnDeleteBranch_Lambda([this](const FString& BranchName)
                                {
                                    OnDeleteBranch(BranchName);
                                })
                                .OnPushBranch_Lambda([this](const FString& BranchName)
                                {
                                    OnPushBranch(BranchName);
                                })
                                .OnResetBranch_Lambda([this](const FString& BranchName)
                                {
                                    OnResetBranch(BranchName);
                                })]

                        + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 10, 0, 5)
                            [SNew(SSeparator)
                                .Orientation(Orient_Horizontal)]

                        // Sidebar tabs
                        + SVerticalBox::Slot()
                            .FillHeight(1.0f)
                            [SAssignNew(SidebarTabs, SCustomGitSidebarTabs)
                                .StashList(&StashList)
                                .CommitHistoryList(&CommitHistoryList)
                                .CommandHistoryList(&CommandHistoryList)
                                .OnPopStash_Lambda([this](const FString& StashRef)
                                {
                                    OnPopStash(StashRef);
                                })
                                .OnDropStash_Lambda([this](const FString& StashRef)
                                {
                                    OnDropStash(StashRef);
                                })]

                        + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(2, 5)
                            [SNew(SButton)
                                .Text(LOCTEXT("Refresh", "Refresh All"))
                                .OnClicked_Lambda([this]()
                                {
                                    RefreshStatus();
                                    UpdateBranchList();
                                    UpdateStashList();
                                    UpdateCommitHistory();
                                    UpdateCommandHistory();
                                    return FReply::Handled();
                                })]
                    ]
                ]

            // --- Main Content (Right) ---
            + SSplitter::Slot()
                .Value(0.75f)
                [SNew(SVerticalBox)

                    // File list
                    + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [SAssignNew(FileListPanel, SCustomGitFileListPanel)
                            .FileList(&LocalFileList)
                            .ViewModeName(TEXT("Local Changes"))
                            .ViewMode(EGitViewMode::LocalChanges)
                            .OnStageFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnStageFiles(Files);
                            })
                            .OnUnstageFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnUnstageFiles(Files);
                            })
                            .OnLockFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnLockFiles(Files);
                            })
                            .OnUnlockFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnUnlockFiles(Files);
                            })
                            .OnDiscardFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnDiscardFiles(Files);
                            })
                            .OnStashFiles_Lambda([this](const TArray<FString>& Files)
                            {
                                OnStashFiles(Files);
                            })]

                    // Commit panel
                    + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(5)
                        [SAssignNew(CommitPanel, SCustomGitCommitPanel)
                            .UserName(CurrentUserName)
                            .UserEmail(CurrentUserEmail)
                            .OnCommit_Lambda([this](const FString& CommitMessage)
                            {
                                OnCommitRequested(CommitMessage);
                            })]
                ]
        ];

    // Initial data load
    RefreshStatus();
    UpdateBranchList();
    UpdateStashList();
    UpdateCommitHistory();
    UpdateCommandHistory();
    UpdateUserInfo();

    // Set initial view
    OnViewModeChanged(EGitViewMode::LocalChanges);
}

void SCustomGitWindow::RefreshStatus()
{
    LocalFileList.Empty();
    StagedFileList.Empty();
    LockedFileList.Empty();
    HistoryFileList.Empty();

    // 1. Get Status
    TMap<FString, FString> StatusMap;
    FCustomGitOperations::UpdateStatus(TArray<FString>(), StatusMap);

    UE_LOG(LogTemp, Log, TEXT("CustomGit: RefreshStatus found %d files in git status"), StatusMap.Num());

    // 2. Parse Status
    for (const auto& Pair : StatusMap)
    {
        FString Filename = Pair.Key;
        FString RawStatus = Pair.Value;

        // Determine if Staged or Unstaged
        // XY format. X = Staged, Y = Unstaged.
        // If RawStatus is less than 2 chars, pad it.
        while (RawStatus.Len() < 2)
            RawStatus += " ";

        TCHAR X = RawStatus[0];
        TCHAR Y = RawStatus[1];

        FDateTime ModTime = FCustomGitOperations::GetFileLastModified(Filename);

        // Check Staged (X)
        if (X != ' ' && X != '?')
        {
            TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
            Item->Filename = Filename;
            Item->Status = FString::Printf(TEXT("Staged (%c)"), X);
            Item->ModificationTime = ModTime;
            StagedFileList.Add(Item);
        }

        // Check Unstaged (Y) or Untracked (??)
        if (Y != ' ' || (X == '?' && Y == '?'))
        {
            TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
            Item->Filename = Filename;
            Item->Status = (X == '?' && Y == '?') ? TEXT("Untracked") : FString::Printf(TEXT("Modified (%c)"), Y);
            Item->ModificationTime = ModTime;
            LocalFileList.Add(Item);
        }
    }

    // 3. Get Locks
    TMap<FString, FString> Locks;
    FCustomGitOperations::GetAllLocks(Locks);
    for (const auto& Pair : Locks)
    {
        TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
        Item->Filename = Pair.Key;
        Item->Status = FString::Printf(TEXT("Locked by %s"), *Pair.Value);
        Item->ModificationTime = FCustomGitOperations::GetFileLastModified(Pair.Key);
        LockedFileList.Add(Item);
    }

    // 4. History
    const TArray<FString>& Hist = FCustomGitOperations::GetCommandHistory();
    // Show in reverse order (newest first)
    for (int32 i = Hist.Num() - 1; i >= 0; i--)
    {
        TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
        Item->Filename = Hist[i]; // The full command string
        Item->Status = TEXT("CMD");
        HistoryFileList.Add(Item);
    }

    if (FileListPanel.IsValid())
    {
        FileListPanel->RefreshList();
    }
}

void SCustomGitWindow::UpdateBranchList()
{
    BranchList.Empty();
    TArray<FGitBranchInfo> Branches;
    FCustomGitOperations::GetBranches(Branches, CurrentBranchName);

    for (const FGitBranchInfo& Branch : Branches)
    {
        BranchList.Add(MakeShareable(new FGitBranchInfo(Branch)));
    }

    if (BranchPanel.IsValid())
    {
        BranchPanel->RefreshBranchList();
        BranchPanel->SetCurrentBranchName(CurrentBranchName);
    }

    if (TopActionsBar.IsValid())
    {
        TopActionsBar->SetBranchName(CurrentBranchName);
    }
}

void SCustomGitWindow::UpdateUserInfo()
{
    FCustomGitOperations::GetUserInfo(CurrentUserName, CurrentUserEmail);

    if (CommitPanel.IsValid())
    {
        CommitPanel->SetUserInfo(CurrentUserName, CurrentUserEmail);
    }
}

void SCustomGitWindow::UpdateCommitHistory()
{
    CommitHistoryList.Empty();
    TArray<FString> Commits;
    FCustomGitOperations::GetCommitHistory(50, Commits);

    for (const FString& Commit : Commits)
    {
        CommitHistoryList.Add(MakeShareable(new FString(Commit)));
    }

    if (SidebarTabs.IsValid())
    {
        SidebarTabs->RefreshCommitHistory();
    }
}

void SCustomGitWindow::UpdateCommandHistory()
{
    CommandHistoryList.Empty();
    const TArray<FString>& Hist = FCustomGitOperations::GetCommandHistory();

    // Show in reverse order (newest first)
    for (int32 i = Hist.Num() - 1; i >= 0; i--)
    {
        CommandHistoryList.Add(MakeShareable(new FString(Hist[i])));
    }

    if (SidebarTabs.IsValid())
    {
        SidebarTabs->RefreshCommandHistory();
    }
}

void SCustomGitWindow::UpdateStashList()
{
    StashList.Empty();
    TArray<FString> Results, Errors;

    // git stash list
    if (FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("list")}, {}, Results, Errors))
    {
        for (const FString& Line : Results)
        {
            if (!Line.IsEmpty())
            {
                StashList.Add(MakeShareable(new FString(Line)));
            }
        }
    }

    if (SidebarTabs.IsValid())
    {
        SidebarTabs->RefreshStashes();
    }
}

void SCustomGitWindow::OnViewModeChanged(EGitViewMode NewMode)
{
    CurrentViewMode = NewMode;

    if (!FileListPanel.IsValid())
    {
        return;
    }

    FileListPanel->SetViewMode(NewMode);

    // Switch file list based on view mode
    switch (NewMode)
    {
    case EGitViewMode::LocalChanges:
        FileListPanel->SetFileList(&LocalFileList);
        CommitPanel->SetVisibility(EVisibility::Collapsed);
        break;
    case EGitViewMode::StagedChanges:
        FileListPanel->SetFileList(&StagedFileList);
        CommitPanel->SetVisibility(EVisibility::Visible);
        break;
    case EGitViewMode::LockedFiles:
        FileListPanel->SetFileList(&LockedFileList);
        CommitPanel->SetVisibility(EVisibility::Collapsed);
        break;
    case EGitViewMode::History:
        FileListPanel->SetFileList(&HistoryFileList);
        CommitPanel->SetVisibility(EVisibility::Collapsed);
        break;
    }
}

void SCustomGitWindow::OnCommitRequested(const FString& CommitMessage)
{
    if (CommitMessage.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoCommitMessage", "Please enter a commit message."));
        return;
    }

    TArray<FString> FilesToCommit;

    // In Staged view, commit all staged files
    if (CurrentViewMode == EGitViewMode::StagedChanges && StagedFileList.Num() > 0)
    {
        for (const auto& FileItem : StagedFileList)
        {
            FilesToCommit.Add(FileItem->Filename);
        }
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoFilesToCommit", "No files to commit. Switch to Staged Changes view."));
        return;
    }

    FString Error;
    if (FCustomGitOperations::Commit(CommitMessage, FilesToCommit, Error))
    {
        CommitPanel->ClearCommitMessage();
        RefreshStatus();
    }
    else if (!Error.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            FText::FromString(FString::Printf(TEXT("Commit failed: %s"), *Error)));
    }
}

void SCustomGitWindow::OnSyncClicked()
{
    TArray<FString> Results, Errors;

    // Pull first
    FCustomGitOperations::RunGitCommand(TEXT("pull"), {}, {}, Results, Errors);

    // Then push
    FCustomGitOperations::RunGitCommand(TEXT("push"), {}, {}, Results, Errors);

    RefreshStatus();
    UpdateBranchList();
}

void SCustomGitWindow::OnResetClicked()
{
    // Show confirmation dialog - this is a destructive operation
    FText Message = LOCTEXT("ResetHardConfirm",
        "Are you sure you want to reset hard to origin?\n\n"
        "This will:\n"
        "- Discard ALL local changes\n"
        "- Reset to the remote branch state\n\n"
        "This cannot be undone!");

    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
    if (Result == EAppReturnType::Yes)
    {
        TArray<FString> Results, Errors;

        // First fetch to make sure we have latest
        FCustomGitOperations::RunGitCommand(TEXT("fetch"), {TEXT("origin")}, {}, Results, Errors);

        // Get current branch name
        Results.Empty();
        Errors.Empty();
        FCustomGitOperations::RunGitCommand(TEXT("rev-parse"), {TEXT("--abbrev-ref"), TEXT("HEAD")}, {}, Results, Errors);

        FString CurrentBranch = Results.Num() > 0 ? Results[0].TrimStartAndEnd() : TEXT("HEAD");

        // Reset hard to origin/branch
        Results.Empty();
        Errors.Empty();
        FString OriginBranch = FString::Printf(TEXT("origin/%s"), *CurrentBranch);
        FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("--hard"), OriginBranch}, {}, Results, Errors);

        RefreshStatus();
        UpdateBranchList();
    }
}

void SCustomGitWindow::OnStageFiles(const TArray<FString>& Files)
{
    TArray<FString> Results, Errors;
    for (const FString& File : Files)
    {
        FCustomGitOperations::RunGitCommand(TEXT("add"), {TEXT("--")}, {File}, Results, Errors);
    }
    RefreshStatus();
}

void SCustomGitWindow::OnUnstageFiles(const TArray<FString>& Files)
{
    TArray<FString> Results, Errors;
    for (const FString& File : Files)
    {
        FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("HEAD"), TEXT("--")}, {File}, Results, Errors);
    }
    RefreshStatus();
}

void SCustomGitWindow::OnLockFiles(const TArray<FString>& Files)
{
    for (const FString& File : Files)
    {
        FString Error;
        FCustomGitOperations::LockFile(File, Error);
        if (!Error.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("CustomGit: Failed to lock %s: %s"), *File, *Error);
        }
    }
    RefreshStatus();
}

void SCustomGitWindow::OnUnlockFiles(const TArray<FString>& Files)
{
    for (const FString& File : Files)
    {
        FString Error;
        FCustomGitOperations::UnlockFile(File, Error);
        if (!Error.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("CustomGit: Failed to unlock %s: %s"), *File, *Error);
        }
    }
    RefreshStatus();
}

void SCustomGitWindow::OnDiscardFiles(const TArray<FString>& Files)
{
    TArray<FString> Results, Errors;
    TMap<FString, FString> StatusMap;
    
    // Get status of all files to determine how to discard them
    FCustomGitOperations::UpdateStatus(Files, StatusMap);

    for (const FString& File : Files)
    {
        // Check if untracked by looking up the status
        FString* Status = StatusMap.Find(File);
        if (Status && Status->Contains(TEXT("??")))
        {
            // Untracked - delete
            FString FullPath = FPaths::Combine(FCustomGitOperations::GetRepositoryRoot(), File);
            IFileManager::Get().Delete(*FullPath);
        }
        else
        {
            // Tracked - checkout
            FCustomGitOperations::RunGitCommand(TEXT("checkout"), {TEXT("--")}, {File}, Results, Errors);
        }
    }

    RefreshStatus();
}

void SCustomGitWindow::OnStashFiles(const TArray<FString>& Files)
{
    TArray<FString> Results, Errors;
    TArray<FString> Params;
    Params.Add(TEXT("push"));
    Params.Add(TEXT("-m"));
    Params.Add(FString::Printf(TEXT("\"Stashed %d files\""), Files.Num()));
    Params.Add(TEXT("--"));

    FCustomGitOperations::RunGitCommand(TEXT("stash"), Params, Files, Results, Errors);

    RefreshStatus();
    UpdateStashList();
}

void SCustomGitWindow::OnSwitchBranch(const FString& BranchName)
{
    FString Error;
    FCustomGitOperations::SwitchBranch(BranchName, Error);
    RefreshStatus();
    UpdateBranchList();
}

void SCustomGitWindow::OnCreateBranch(const FString& BranchName)
{
    FString Error;
    if (FCustomGitOperations::CreateBranch(BranchName, Error))
    {
        FCustomGitOperations::SwitchBranch(BranchName, Error);
        RefreshStatus();
        UpdateBranchList();
    }
}

void SCustomGitWindow::OnDeleteBranch(const FString& BranchName)
{
    TArray<FString> Results, Errors;
    FCustomGitOperations::RunGitCommand(TEXT("branch"), {TEXT("-d"), BranchName}, {}, Results, Errors);
    UpdateBranchList();
}

void SCustomGitWindow::OnPushBranch(const FString& BranchName)
{
    TArray<FString> Results, Errors;

    // Check if it has upstream
    FCustomGitOperations::RunGitCommand(TEXT("config"), {FString::Printf(TEXT("branch.%s.remote"), *BranchName)}, {},
        Results, Errors);
    bool bHasUpstream = (Results.Num() > 0 && !Results[0].IsEmpty());

    Results.Empty();
    Errors.Empty();

    if (!bHasUpstream)
    {
        // First time push - use -u to set upstream
        FCustomGitOperations::RunGitCommand(TEXT("push"), {TEXT("-u"), TEXT("origin"), BranchName}, {}, Results, Errors);
    }
    else
    {
        // Regular push
        FCustomGitOperations::RunGitCommand(TEXT("push"), {TEXT("origin"), BranchName}, {}, Results, Errors);
    }

    UpdateBranchList();
}

void SCustomGitWindow::OnResetBranch(const FString& BranchName)
{
    TArray<FString> Results, Errors;
    FString SwitchError;

    // First switch to the branch if not current
    FCustomGitOperations::SwitchBranch(BranchName, SwitchError);

    // Fetch latest
    FCustomGitOperations::RunGitCommand(TEXT("fetch"), {TEXT("origin")}, {}, Results, Errors);

    // Reset hard to origin
    FString OriginBranch = FString::Printf(TEXT("origin/%s"), *BranchName);
    FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("--hard"), OriginBranch}, {}, Results, Errors);

    RefreshStatus();
    UpdateBranchList();
}

void SCustomGitWindow::OnPopStash(const FString& StashRef)
{
    TArray<FString> Results, Errors;
    FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("pop"), StashRef}, {}, Results, Errors);
    RefreshStatus();
    UpdateStashList();
}

void SCustomGitWindow::OnDropStash(const FString& StashRef)
{
    TArray<FString> Results, Errors;
    FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("drop"), StashRef}, {}, Results, Errors);
    UpdateStashList();
}

#undef LOCTEXT_NAMESPACE
