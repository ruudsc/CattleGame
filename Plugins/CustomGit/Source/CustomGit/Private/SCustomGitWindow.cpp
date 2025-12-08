#include "SCustomGitWindow.h"
#include "CustomGitOperations.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SDropTarget.h"
#include "EditorStyleSet.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SCustomGitWindow"

void SCustomGitWindow::Construct(const FArguments &InArgs)
{
    CurrentViewMode = EGitViewMode::LocalChanges;
    CurrentFileList = &LocalFileList;

    // Initialize view mode list
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Local Changes"), EGitViewMode::LocalChanges}));
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Staged Changes"), EGitViewMode::StagedChanges}));
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Locked Files"), EGitViewMode::LockedFiles}));

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

                             // Sync Button at top
                             + SVerticalBox::Slot().AutoHeight().Padding(2)
                                   [SAssignNew(SyncButton, SButton)
                                        .Text(LOCTEXT("SyncBtn", "Sync (Push/Pull)"))
                                        .OnClicked(this, &SCustomGitWindow::OnSyncClicked)]

                             + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 5)
                                   [SNew(SSeparator).Orientation(Orient_Horizontal)]

                             // Current Changes Header
                             + SVerticalBox::Slot().AutoHeight().Padding(2)
                                   [SNew(STextBlock).Text(LOCTEXT("CurrentChangesHeader", "Current Changes")).Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))]

                             // View Mode List (Local Changes, Staged, Locked)
                             + SVerticalBox::Slot().AutoHeight()
                                   [SAssignNew(ViewModeListView, SListView<TSharedPtr<FViewModeItem>>)
                                        .ListItemsSource(&ViewModeList)
                                        .OnGenerateRow(this, &SCustomGitWindow::OnGenerateViewModeRow)
                                        .OnSelectionChanged(this, &SCustomGitWindow::OnViewModeSelectionChanged)
                                        .SelectionMode(ESelectionMode::Single)]

                             + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
                                   [SNew(SSeparator).Orientation(Orient_Horizontal)]

                             // Branches
                             + SVerticalBox::Slot().AutoHeight().Padding(2)
                                   [SNew(STextBlock).Text(LOCTEXT("BranchesHeader", "Branches")).Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))] +
                             SVerticalBox::Slot().FillHeight(0.5f)
                                 [SAssignNew(BranchListView, SListView<TSharedPtr<FGitBranchInfo>>)
                                      .ListItemsSource(&BranchList)
                                      .OnGenerateRow(this, &SCustomGitWindow::OnGenerateBranchRow)
                                      .OnContextMenuOpening(this, &SCustomGitWindow::OnOpenBranchContextMenu)]

                             // Create Branch
                             + SVerticalBox::Slot().AutoHeight().Padding(2, 5)
                                   [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.0f)[SAssignNew(NewBranchNameInput, SEditableTextBox).HintText(LOCTEXT("NewBranchHint", "New Branch Name"))] + SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)[SNew(SButton).Text(LOCTEXT("Create", "+")).OnClicked(this, &SCustomGitWindow::OnCreateBranchClicked)]]

                             + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
                                   [SNew(SSeparator).Orientation(Orient_Horizontal)]

                             // Sidebar Tabs (Stashes / Commit History / Command History)
                             + SVerticalBox::Slot().AutoHeight().Padding(2)
                                   [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)[SNew(SButton).Text(LOCTEXT("TabStashes", "Stashes")).OnClicked_Lambda([this]()
                                                                                                                                                                                   { OnSidebarTabChanged(ESidebarTab::Stashes); return FReply::Handled(); })] +
                                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                                        [SNew(SButton)
                                             .Text(LOCTEXT("TabCommits", "Commits"))
                                             .OnClicked_Lambda([this]()
                                                               { OnSidebarTabChanged(ESidebarTab::CommitHistory); return FReply::Handled(); })] +
                                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                                        [SNew(SButton)
                                             .Text(LOCTEXT("TabCmdHist", "Cmds"))
                                             .OnClicked_Lambda([this]()
                                                               { OnSidebarTabChanged(ESidebarTab::CommandHistory); return FReply::Handled(); })]] +
                             SVerticalBox::Slot().FillHeight(0.5f)
                                 [SAssignNew(SidebarListView, SListView<TSharedPtr<FString>>)
                                      .ListItemsSource(&StashList)
                                      .OnGenerateRow(this, &SCustomGitWindow::OnGenerateSidebarRow)]

                             + SVerticalBox::Slot().AutoHeight().Padding(2, 5)
                                   [SNew(SButton).Text(LOCTEXT("Refresh", "Refresh All")).OnClicked(this, &SCustomGitWindow::OnRefreshClicked)]]]

         // --- Main Content (Right) ---
         + SSplitter::Slot()
               .Value(0.75f)
                   [SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1.0f)[SAssignNew(ListView, SListView<TSharedPtr<FGitFileStatus>>).ListItemsSource(CurrentFileList).SelectionMode(ESelectionMode::Multi).OnGenerateRow(this, &SCustomGitWindow::OnGenerateRow).OnContextMenuOpening(this, &SCustomGitWindow::OnOpenContextMenu).HeaderRow(SNew(SHeaderRow) + SHeaderRow::Column("Status").DefaultLabel(LOCTEXT("StatusCol", "Status")).FixedWidth(60) + SHeaderRow::Column("File").DefaultLabel(LOCTEXT("FileCol", "File")).FillWidth(1.0f) + SHeaderRow::Column("Date").DefaultLabel(LOCTEXT("DateCol", "Last Modified")).FixedWidth(120))]

                    // Context Action Area (Commit)
                    + SVerticalBox::Slot()
                          .AutoHeight()
                          .Padding(5)
                              [SAssignNew(CommitBox, SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)[SAssignNew(AuthorTextBlock, STextBlock).Text(LOCTEXT("AuthorPlaceholder", "Author: Loading..."))] + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CommitMsg", "Commit Message:"))] + SVerticalBox::Slot().AutoHeight().Padding(0, 2)[SAssignNew(CommitMessageInput, SMultiLineEditableTextBox).HintText(LOCTEXT("CommitHint", "Enter commit message..."))] + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 2)[SNew(SButton).Text(LOCTEXT("CommitBtn", "Commit Changes")).OnClicked(this, &SCustomGitWindow::OnCommitClicked)]]]];

    RefreshStatus();
    UpdateBranchList();
    UpdateStashList();
    UpdateCommitHistory();
    UpdateCommandHistory();
    UpdateUserInfo();

    // Default view
    OnViewLocalClicked(); // Set initial view properly
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
    for (const auto &Pair : StatusMap)
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
    for (const auto &Pair : Locks)
    {
        TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
        Item->Filename = Pair.Key;
        Item->Status = FString::Printf(TEXT("Locked by %s"), *Pair.Value);
        Item->ModificationTime = FCustomGitOperations::GetFileLastModified(Pair.Key);
        LockedFileList.Add(Item);
    }

    // 4. History
    const TArray<FString> &Hist = FCustomGitOperations::GetCommandHistory();
    // Show in reverse order (newest first)
    for (int32 i = Hist.Num() - 1; i >= 0; i--)
    {
        TSharedPtr<FGitFileStatus> Item = MakeShareable(new FGitFileStatus());
        Item->Filename = Hist[i]; // The full command string
        Item->Status = TEXT("CMD");
        HistoryFileList.Add(Item);
    }

    if (ListView)
    {
        ListView->RequestListRefresh();
    }
}

void SCustomGitWindow::UpdateBranchList()
{
    BranchList.Empty();
    TArray<FGitBranchInfo> Branches;
    FCustomGitOperations::GetBranches(Branches, CurrentBranchName);

    for (const FGitBranchInfo &Branch : Branches)
    {
        BranchList.Add(MakeShareable(new FGitBranchInfo(Branch)));
    }

    if (BranchListView)
    {
        BranchListView->RequestListRefresh();
    }

    // Update sync button text with current branch name
    if (SyncButton.IsValid() && !CurrentBranchName.IsEmpty())
    {
        FString SyncText = FString::Printf(TEXT("Sync (Push/Pull) - %s"), *CurrentBranchName);
        SyncButton->SetContent(
            SNew(STextBlock)
                .Text(FText::FromString(SyncText)));
    }
}

void SCustomGitWindow::UpdateUserInfo()
{
    FCustomGitOperations::GetUserInfo(CurrentUserName, CurrentUserEmail);

    if (AuthorTextBlock.IsValid())
    {
        FString AuthorText = FString::Printf(TEXT("Author: %s <%s>"), *CurrentUserName, *CurrentUserEmail);
        AuthorTextBlock->SetText(FText::FromString(AuthorText));
    }
}

FReply SCustomGitWindow::OnCreateBranchClicked()
{
    FString NewName = NewBranchNameInput->GetText().ToString();
    if (NewName.IsEmpty())
        return FReply::Handled();

    FString Error;
    if (FCustomGitOperations::CreateBranch(NewName, Error))
    {
        FCustomGitOperations::SwitchBranch(NewName, Error);
        NewBranchNameInput->SetText(FText::GetEmpty());
        OnRefreshClicked();
    }

    return FReply::Handled();
}

FReply SCustomGitWindow::OnRefreshClicked()
{
    RefreshStatus();
    UpdateBranchList();
    UpdateStashList();
    return FReply::Handled();
}

FReply SCustomGitWindow::OnViewLocalClicked()
{
    CurrentViewMode = EGitViewMode::LocalChanges;
    CurrentFileList = &LocalFileList;
    if (ListView)
        ListView->SetItemsSource(CurrentFileList);

    // Update view mode list selection
    if (ViewModeListView.IsValid() && ViewModeList.Num() > 0)
    {
        ViewModeListView->SetSelection(ViewModeList[0], ESelectInfo::Direct);
        ViewModeListView->RequestListRefresh();
    }

    // Hide Commit Box
    if (CommitBox)
        CommitBox->SetVisibility(EVisibility::Collapsed);

    return FReply::Handled();
}

FReply SCustomGitWindow::OnViewStagedClicked()
{
    CurrentViewMode = EGitViewMode::StagedChanges;
    CurrentFileList = &StagedFileList;
    if (ListView)
        ListView->SetItemsSource(CurrentFileList);

    // Update view mode list selection
    if (ViewModeListView.IsValid() && ViewModeList.Num() > 1)
    {
        ViewModeListView->SetSelection(ViewModeList[1], ESelectInfo::Direct);
        ViewModeListView->RequestListRefresh();
    }

    // Show Commit Box
    if (CommitBox)
        CommitBox->SetVisibility(EVisibility::Visible);

    return FReply::Handled();
}

FReply SCustomGitWindow::OnViewLockedClicked()
{
    CurrentViewMode = EGitViewMode::LockedFiles;
    CurrentFileList = &LockedFileList;
    if (ListView)
        ListView->SetItemsSource(CurrentFileList);

    // Update view mode list selection
    if (ViewModeListView.IsValid() && ViewModeList.Num() > 2)
    {
        ViewModeListView->SetSelection(ViewModeList[2], ESelectInfo::Direct);
        ViewModeListView->RequestListRefresh();
    }

    if (CommitBox)
        CommitBox->SetVisibility(EVisibility::Collapsed);

    return FReply::Handled();
}

FReply SCustomGitWindow::OnViewHistoryClicked()
{
    CurrentViewMode = EGitViewMode::History;
    CurrentFileList = &HistoryFileList;
    if (ListView)
        ListView->SetItemsSource(CurrentFileList);

    if (CommitBox)
        CommitBox->SetVisibility(EVisibility::Collapsed);

    return FReply::Handled();
}

FReply SCustomGitWindow::OnCommitClicked()
{
    // If viewing Local, we might auto-stage?
    // If viewing Staged, we commit them?
    // User expectation: "Submit content"
    // Git workflow: Stage then Commit.

    // Simplification: Commit all SELECTED files in the current view.
    // If in Local view -> Stage + Commit? Or just Commit directly (git commit file list).
    // If in Staged view -> Commit.

    TArray<FString> FilesToCommit;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &FileItem : SelectedItems)
    {
        FilesToCommit.Add(FileItem->Filename);
    }

    if (FilesToCommit.Num() == 0)
        return FReply::Handled();

    FString Msg = CommitMessageInput->GetText().ToString();
    if (Msg.IsEmpty())
        return FReply::Handled();

    FString Error;
    if (FCustomGitOperations::Commit(Msg, FilesToCommit, Error))
    {
        CommitMessageInput->SetText(FText::GetEmpty());
        OnRefreshClicked();
    }

    return FReply::Handled();
}

TSharedRef<ITableRow> SCustomGitWindow::OnGenerateRow(TSharedPtr<FGitFileStatus> InItem, const TSharedRef<STableViewBase> &OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FGitFileStatus>>, OwnerTable)
        .OnDragDetected(this, &SCustomGitWindow::OnListDragDetected)
            [SNew(SHorizontalBox) +
             SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
                 [SNew(SBox).WidthOverride(60)
                      [SNew(STextBlock).Text(FText::FromString(InItem->Status))]] +
             SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0)
                 [SNew(STextBlock).Text(FText::FromString(InItem->Filename))] +
             SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
                 [SNew(SBox).WidthOverride(120)
                      [SNew(STextBlock).Text(FText::FromString(InItem->ModificationTime.ToHttpDate())) // Use simpler date format or custom
    ]]];
}

TSharedRef<ITableRow> SCustomGitWindow::OnGenerateBranchRow(TSharedPtr<FGitBranchInfo> InItem, const TSharedRef<STableViewBase> &OwnerTable)
{
    // Build status suffix text and color
    FString StatusSuffix;
    FSlateColor StatusColor = FSlateColor(FLinearColor::Gray);

    if (InItem->bIsUpstreamGone)
    {
        StatusSuffix = TEXT(" (upstream gone)");
        StatusColor = FSlateColor(FLinearColor::Red);
    }
    else if (InItem->bIsLocal)
    {
        StatusSuffix = TEXT(" (local)");
        StatusColor = FSlateColor(FLinearColor::Gray);
    }

    return SNew(STableRow<TSharedPtr<FGitBranchInfo>>, OwnerTable)
        [SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)[SNew(STextBlock).Text(InItem->bIsCurrent ? FText::FromString(TEXT("*")) : FText::GetEmpty()).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))] + SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(InItem->Name)).Font(InItem->bIsCurrent ? FCoreStyle::GetDefaultFontStyle("Bold", 10) : FCoreStyle::GetDefaultFontStyle("Regular", 10))] + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0, 5, 0)[SNew(STextBlock).Text(FText::FromString(StatusSuffix)).Font(FCoreStyle::GetDefaultFontStyle("Italic", 9)).ColorAndOpacity(StatusColor)]];
}

TSharedPtr<SWidget> SCustomGitWindow::OnOpenBranchContextMenu()
{
    TArray<TSharedPtr<FGitBranchInfo>> SelectedBranches = BranchListView->GetSelectedItems();
    if (SelectedBranches.Num() == 0)
    {
        return nullptr;
    }

    FString BranchName = SelectedBranches[0]->Name;

    // Check if branch has upstream tracking
    TArray<FString> UpstreamResults, UpstreamErrors;
    bool bHasUpstream = false;
    FCustomGitOperations::RunGitCommand(TEXT("config"), {FString::Printf(TEXT("branch.%s.remote"), *BranchName)}, {}, UpstreamResults, UpstreamErrors);
    bHasUpstream = (UpstreamResults.Num() > 0 && !UpstreamResults[0].IsEmpty());

    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.AddMenuEntry(
        LOCTEXT("SwitchBranch", "Switch to Branch"),
        LOCTEXT("SwitchBranchTooltip", "Switch to the selected branch"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
                                         {
                if (SelectedBranches.Num() > 0)
                {
                    FString Error;
                    FCustomGitOperations::SwitchBranch(SelectedBranches[0]->Name, Error);
                    OnRefreshClicked();
                } })));

    MenuBuilder.AddSeparator();

    // Show "Push branch to origin" only if no upstream exists (local-only branch)
    if (!bHasUpstream)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("PushBranchToOrigin", "Push Branch to Origin"),
            LOCTEXT("PushBranchToOriginTooltip", "Push this local branch to origin for the first time"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this, BranchName]()
                                             {
                    TArray<FString> Results, Errors;
                    // Push with -u to set upstream tracking
                    FCustomGitOperations::RunGitCommand(TEXT("push"), {TEXT("-u"), TEXT("origin"), BranchName}, {}, Results, Errors);
                    UpdateBranchList();
                    OnRefreshClicked();
                })));
    }

    // Always show Push option
    MenuBuilder.AddMenuEntry(
        LOCTEXT("PushBranch", "Push"),
        LOCTEXT("PushBranchTooltip", "Push commits on this branch to origin"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, BranchName]()
                                         {
                TArray<FString> Results, Errors;
                FCustomGitOperations::RunGitCommand(TEXT("push"), {TEXT("origin"), BranchName}, {}, Results, Errors);
                OnRefreshClicked();
            })));

    MenuBuilder.AddSeparator();

    MenuBuilder.AddMenuEntry(
        LOCTEXT("DeleteBranch", "Delete Branch"),
        LOCTEXT("DeleteBranchTooltip", "Delete the selected branch"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
                                         {
                if (SelectedBranches.Num() > 0)
                {
                    FText Message = FText::Format(
                        LOCTEXT("DeleteBranchConfirm", "Are you sure you want to delete branch '{0}'?"),
                        FText::FromString(SelectedBranches[0]->Name));
                    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
                    if (Result == EAppReturnType::Yes)
                    {
                        TArray<FString> Results, Errors;
                        FCustomGitOperations::RunGitCommand(TEXT("branch"), {TEXT("-d"), SelectedBranches[0]->Name}, {}, Results, Errors);
                        UpdateBranchList();
                    }
                } })));

    MenuBuilder.AddSeparator();

    MenuBuilder.AddMenuEntry(
        LOCTEXT("ResetToOrigin", "Reset Hard to Origin"),
        LOCTEXT("ResetToOriginTooltip", "Reset the selected branch to match origin (DESTRUCTIVE)"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
                                         {
                if (SelectedBranches.Num() > 0)
                {
                    FText Message = FText::Format(
                        LOCTEXT("ResetHardConfirm", "WARNING: This will PERMANENTLY discard all local changes on branch '{0}' and reset to origin.\n\nAre you sure?"),
                        FText::FromString(SelectedBranches[0]->Name));
                    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
                    if (Result == EAppReturnType::Yes)
                    {
                        // First switch to the branch if not current
                        if (!SelectedBranches[0]->bIsCurrent)
                        {
                            FString Error;
                            FCustomGitOperations::SwitchBranch(SelectedBranches[0]->Name, Error);
                        }
                        
                        // Fetch latest
                        TArray<FString> Results, Errors;
                        FCustomGitOperations::RunGitCommand(TEXT("fetch"), {TEXT("origin")}, {}, Results, Errors);
                        
                        // Reset hard to origin
                        FString OriginBranch = FString::Printf(TEXT("origin/%s"), *SelectedBranches[0]->Name);
                        FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("--hard"), OriginBranch}, {}, Results, Errors);
                        
                        OnRefreshClicked();
                    }
                } })));

    return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SCustomGitWindow::OnGenerateViewModeRow(TSharedPtr<FViewModeItem> InItem, const TSharedRef<STableViewBase> &OwnerTable)
{
    bool bIsSelected = (CurrentViewMode == InItem->ViewMode);

    return SNew(STableRow<TSharedPtr<FViewModeItem>>, OwnerTable)
        [SNew(SBorder)
             .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
             .Padding(FMargin(5, 2))
                 [SNew(STextBlock)
                      .Text(FText::FromString(InItem->Name))
                      .Font(bIsSelected ? FCoreStyle::GetDefaultFontStyle("Bold", 10) : FCoreStyle::GetDefaultFontStyle("Regular", 10))]];
}

void SCustomGitWindow::OnViewModeSelectionChanged(TSharedPtr<FViewModeItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (!SelectedItem.IsValid() || SelectInfo == ESelectInfo::Direct)
    {
        return;
    }

    switch (SelectedItem->ViewMode)
    {
    case EGitViewMode::LocalChanges:
        OnViewLocalClicked();
        break;
    case EGitViewMode::StagedChanges:
        OnViewStagedClicked();
        break;
    case EGitViewMode::LockedFiles:
        OnViewLockedClicked();
        break;
    default:
        break;
    }
}

FReply SCustomGitWindow::OnListDragDetected(const FGeometry &MyGeometry, const FPointerEvent &MouseEvent)
{
    // Allow dragging from both Local Changes and Staged Changes views
    if (CurrentViewMode != EGitViewMode::LocalChanges && CurrentViewMode != EGitViewMode::StagedChanges)
    {
        return FReply::Unhandled();
    }

    TArray<FString> SelectedFiles;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        SelectedFiles.Add(Item->Filename);
    }

    if (SelectedFiles.Num() > 0)
    {
        return FReply::Handled().BeginDragDrop(FGitFileDragDropOp::New(SelectedFiles));
    }

    return FReply::Unhandled();
}

FReply SCustomGitWindow::OnStagedDrop(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent)
{
    TSharedPtr<FGitFileDragDropOp> DragOp = DragDropEvent.GetOperationAs<FGitFileDragDropOp>();
    if (DragOp.IsValid())
    {
        // Stage these files individually
        TArray<FString> Results, Errors;
        for (const FString &File : DragOp->FilesToStage)
        {
            FCustomGitOperations::RunGitCommand(TEXT("add"), {TEXT("--")}, {File}, Results, Errors);
        }

        RefreshStatus();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void SCustomGitWindow::OnStagedDragEnter(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent)
{
    // Optional: Highlight button
}

void SCustomGitWindow::OnStagedDragLeave(const FDragDropEvent &DragDropEvent)
{
    // Optional: Unhighlight button
}

FReply SCustomGitWindow::OnLocalDrop(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent)
{
    TSharedPtr<FGitFileDragDropOp> DragOp = DragDropEvent.GetOperationAs<FGitFileDragDropOp>();
    if (DragOp.IsValid())
    {
        // Unstage these files individually using git reset HEAD
        TArray<FString> Results, Errors;
        for (const FString &File : DragOp->FilesToStage)
        {
            FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("HEAD"), TEXT("--")}, {File}, Results, Errors);
        }

        RefreshStatus();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

TSharedPtr<SWidget> SCustomGitWindow::OnOpenContextMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    if (CurrentViewMode == EGitViewMode::LocalChanges)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("StageSelected", "Stage Selected Files"),
            LOCTEXT("StageSelectedTooltip", "Add selected files to staging area"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextStageSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("StashSelected", "Stash Selected Files"),
            LOCTEXT("StashSelectedTooltip", "Stash selected files for later"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextStashSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("DiscardChanges", "Discard Changes"),
            LOCTEXT("DiscardChangesTooltip", "Discard changes to selected files (git checkout)"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextDiscardChanges)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("LockSelected", "Lock Selected Files"),
            LOCTEXT("LockSelectedTooltip", "Lock selected files with Git LFS"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextLockSelected)));
    }
    else if (CurrentViewMode == EGitViewMode::StagedChanges)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("UnstageSelected", "Unstage Selected Files"),
            LOCTEXT("UnstageSelectedTooltip", "Remove selected files from staging area"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextUnstageSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("LockSelectedStaged", "Lock Selected Files"),
            LOCTEXT("LockSelectedStagedTooltip", "Lock selected files with Git LFS"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextLockSelected)));
    }
    else if (CurrentViewMode == EGitViewMode::LockedFiles)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("ReleaseLock", "Release Lock"),
            LOCTEXT("ReleaseLockTooltip", "Release the LFS lock on selected files"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &SCustomGitWindow::OnContextReleaseLock)));
    }
    else
    {
        return nullptr;
    }

    return MenuBuilder.MakeWidget();
}

void SCustomGitWindow::OnContextStageSelected()
{
    TArray<FString> FilesToStage;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToStage.Add(Item->Filename);
    }

    if (FilesToStage.Num() > 0)
    {
        TArray<FString> Results, Errors;
        // Stage each file individually to ensure only selected files are staged
        for (const FString &File : FilesToStage)
        {
            FCustomGitOperations::RunGitCommand(TEXT("add"), {TEXT("--")}, {File}, Results, Errors);
        }
        RefreshStatus();
    }
}

void SCustomGitWindow::OnContextUnstageSelected()
{
    TArray<FString> FilesToUnstage;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToUnstage.Add(Item->Filename);
    }

    if (FilesToUnstage.Num() > 0)
    {
        TArray<FString> Results, Errors;
        // Unstage each file individually using git reset HEAD
        for (const FString &File : FilesToUnstage)
        {
            FCustomGitOperations::RunGitCommand(TEXT("reset"), {TEXT("HEAD"), TEXT("--")}, {File}, Results, Errors);
        }
        RefreshStatus();
    }
}

void SCustomGitWindow::OnContextReleaseLock()
{
    TArray<FString> FilesToUnlock;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToUnlock.Add(Item->Filename);
    }

    if (FilesToUnlock.Num() > 0)
    {
        for (const FString &File : FilesToUnlock)
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
}

void SCustomGitWindow::OnContextStashSelected()
{
    TArray<FString> FilesToStash;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToStash.Add(Item->Filename);
    }

    if (FilesToStash.Num() > 0)
    {
        TArray<FString> Results, Errors;
        // First stage the selected files, then stash them with --keep-index to only stash staged
        // Actually, git stash push can take file paths directly
        TArray<FString> Params;
        Params.Add(TEXT("push"));
        Params.Add(TEXT("-m"));
        Params.Add(FString::Printf(TEXT("\"Stashed %d files\""), FilesToStash.Num()));
        Params.Add(TEXT("--"));

        FCustomGitOperations::RunGitCommand(TEXT("stash"), Params, FilesToStash, Results, Errors);

        RefreshStatus();
        UpdateStashList();
    }
}

void SCustomGitWindow::OnContextLockSelected()
{
    TArray<FString> FilesToLock;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToLock.Add(Item->Filename);
    }

    if (FilesToLock.Num() > 0)
    {
        for (const FString &File : FilesToLock)
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
}

void SCustomGitWindow::UpdateStashList()
{
    StashList.Empty();
    TArray<FString> Results, Errors;

    // git stash list
    if (FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("list")}, {}, Results, Errors))
    {
        for (const FString &Line : Results)
        {
            if (!Line.IsEmpty())
            {
                StashList.Add(MakeShareable(new FString(Line)));
            }
        }
    }

    // Refresh the sidebar list if we're viewing stashes
    if (CurrentSidebarTab == ESidebarTab::Stashes && SidebarListView.IsValid())
    {
        SidebarListView->SetItemsSource(&StashList);
        SidebarListView->RequestListRefresh();
    }
}

void SCustomGitWindow::UpdateCommitHistory()
{
    CommitHistoryList.Empty();
    TArray<FString> Commits;
    FCustomGitOperations::GetCommitHistory(50, Commits);

    for (const FString &Commit : Commits)
    {
        CommitHistoryList.Add(MakeShareable(new FString(Commit)));
    }

    // Refresh the sidebar list if we're viewing commit history
    if (CurrentSidebarTab == ESidebarTab::CommitHistory && SidebarListView.IsValid())
    {
        SidebarListView->SetItemsSource(&CommitHistoryList);
        SidebarListView->RequestListRefresh();
    }
}

void SCustomGitWindow::UpdateCommandHistory()
{
    CommandHistoryList.Empty();
    const TArray<FString> &Hist = FCustomGitOperations::GetCommandHistory();

    // Show in reverse order (newest first)
    for (int32 i = Hist.Num() - 1; i >= 0; i--)
    {
        CommandHistoryList.Add(MakeShareable(new FString(Hist[i])));
    }

    // Refresh the sidebar list if we're viewing command history
    if (CurrentSidebarTab == ESidebarTab::CommandHistory && SidebarListView.IsValid())
    {
        SidebarListView->SetItemsSource(&CommandHistoryList);
        SidebarListView->RequestListRefresh();
    }
}

void SCustomGitWindow::OnSidebarTabChanged(ESidebarTab NewTab)
{
    CurrentSidebarTab = NewTab;

    if (!SidebarListView.IsValid())
        return;

    switch (NewTab)
    {
    case ESidebarTab::Stashes:
        UpdateStashList();
        SidebarListView->SetItemsSource(&StashList);
        break;
    case ESidebarTab::CommitHistory:
        UpdateCommitHistory();
        SidebarListView->SetItemsSource(&CommitHistoryList);
        break;
    case ESidebarTab::CommandHistory:
        UpdateCommandHistory();
        SidebarListView->SetItemsSource(&CommandHistoryList);
        break;
    }

    SidebarListView->RequestListRefresh();
}

TSharedRef<ITableRow> SCustomGitWindow::OnGenerateSidebarRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase> &OwnerTable)
{
    // Different display based on current tab
    if (CurrentSidebarTab == ESidebarTab::Stashes)
    {
        // Parse stash entry: "stash@{0}: WIP on branch: message"
        FString StashRef;
        FString StashMessage = *InItem;

        int32 ColonIndex;
        if (InItem->FindChar(TEXT(':'), ColonIndex))
        {
            StashRef = InItem->Left(ColonIndex);
            StashMessage = InItem->Mid(ColonIndex + 1).TrimStart();
        }

        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(StashMessage)).ToolTipText(FText::FromString(*InItem))] + SHorizontalBox::Slot().AutoWidth().Padding(2)[SNew(SButton).Text(LOCTEXT("PopStash2", "Pop")).OnClicked_Lambda([this, StashRef]()
                                                                                                                                                                                                                                                                                                                             {
                          TArray<FString> Results, Errors;
                          FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("pop"), StashRef}, {}, Results, Errors);
                          RefreshStatus();
                          UpdateStashList();
                          return FReply::Handled(); })] +
             SHorizontalBox::Slot().AutoWidth().Padding(2)
                 [SNew(SButton).Text(LOCTEXT("DropStash2", "Drop")).OnClicked_Lambda([this, StashRef]()
                                                                                     {
                          TArray<FString> Results, Errors;
                          FCustomGitOperations::RunGitCommand(TEXT("stash"), {TEXT("drop"), StashRef}, {}, Results, Errors);
                          UpdateStashList();
                          return FReply::Handled(); })]];
    }
    else
    {
        // Simple text display for commit history and command history
        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5)
                                        [SNew(STextBlock).Text(FText::FromString(*InItem)).ToolTipText(FText::FromString(*InItem))]];
    }
}

void SCustomGitWindow::OnContextDiscardChanges()
{
    TArray<FString> FilesToDiscard;
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
    for (const auto &Item : SelectedItems)
    {
        FilesToDiscard.Add(Item->Filename);
    }

    if (FilesToDiscard.Num() > 0)
    {
        // Show confirmation dialog
        FText Message = FText::Format(
            LOCTEXT("DiscardConfirm", "Are you sure you want to discard changes to {0} file(s)? This cannot be undone."),
            FText::AsNumber(FilesToDiscard.Num()));

        EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
        if (Result == EAppReturnType::Yes)
        {
            TArray<FString> Results, Errors;
            // Use git checkout -- for tracked files
            for (const FString &File : FilesToDiscard)
            {
                FCustomGitOperations::RunGitCommand(TEXT("checkout"), {TEXT("--")}, {File}, Results, Errors);
            }
            RefreshStatus();
        }
    }
}

FReply SCustomGitWindow::OnSyncClicked()
{
    TArray<FString> Results, Errors;

    // Pull first
    FCustomGitOperations::RunGitCommand(TEXT("pull"), {}, {}, Results, Errors);

    // Then push
    FCustomGitOperations::RunGitCommand(TEXT("push"), {}, {}, Results, Errors);

    RefreshStatus();
    UpdateBranchList();

    return FReply::Handled();
}

FReply SCustomGitWindow::OnResetHardClicked()
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

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
