#include "SCustomGitFileListPanel.h"
#include "SCustomGitWindow.h"
#include "CustomGitOperations.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "SCustomGitFileListPanel"

void SCustomGitFileListPanel::Construct(const FArguments& InArgs)
{
    CurrentFileList = InArgs._FileList;
    CurrentViewMode = InArgs._ViewMode;
    OnStageFilesDelegate = InArgs._OnStageFiles;
    OnUnstageFilesDelegate = InArgs._OnUnstageFiles;
    OnLockFilesDelegate = InArgs._OnLockFiles;
    OnUnlockFilesDelegate = InArgs._OnUnlockFiles;
    OnDiscardFilesDelegate = InArgs._OnDiscardFiles;
    OnStashFilesDelegate = InArgs._OnStashFiles;

    ChildSlot
        [SAssignNew(ListView, SListView<TSharedPtr<FGitFileStatus>>)
            .ListItemsSource((CurrentFileList) ? CurrentFileList : nullptr)
            .SelectionMode(ESelectionMode::Multi)
            .OnGenerateRow(this, &SCustomGitFileListPanel::OnGenerateRow)
            .OnContextMenuOpening(this, &SCustomGitFileListPanel::OnOpenContextMenu)
            .HeaderRow(
                SAssignNew(HeaderRow, SHeaderRow)
                + SHeaderRow::Column("Status")
                    .DefaultLabel(LOCTEXT("StatusCol", "Status"))
                    .FixedWidth(60)
                + SHeaderRow::Column("File")
                    .DefaultLabel(LOCTEXT("FileCol", "File"))
                    .FillWidth(1.0f)
                + SHeaderRow::Column("Date")
                    .DefaultLabel(LOCTEXT("DateCol", "Last Modified"))
                    .FixedWidth(120)
            )
        ];
}

TSharedRef<ITableRow> SCustomGitFileListPanel::OnGenerateRow(TSharedPtr<FGitFileStatus> InItem,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FGitFileStatus>>, OwnerTable)
        .OnDragDetected(this, &SCustomGitFileListPanel::OnListDragDetected)
        [SNew(SHorizontalBox)

            // Status column
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [SNew(SBox)
                    .WidthOverride(60)
                    [SNew(STextBlock)
                        .Text(FText::FromString(InItem->Status))]]

            // Filename column
            + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(5, 0)
                [SNew(STextBlock)
                    .Text(FText::FromString(InItem->Filename))]

            // Date column
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [SNew(SBox)
                    .WidthOverride(120)
                    [SNew(STextBlock)
                        .Text(FText::FromString(InItem->ModificationTime.ToHttpDate()))]
                ]
        ];
}

FReply SCustomGitFileListPanel::OnListDragDetected(const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    // Allow dragging from both Local Changes and Staged Changes views
    if (CurrentViewMode != EGitViewMode::LocalChanges && CurrentViewMode != EGitViewMode::StagedChanges)
    {
        return FReply::Unhandled();
    }

    TArray<FString> SelectedFiles;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            SelectedFiles.Add(Item->Filename);
        }
    }

    if (SelectedFiles.Num() > 0)
    {
        return FReply::Handled().BeginDragDrop(FGitFileDragDropOp::New(SelectedFiles));
    }

    return FReply::Unhandled();
}

TSharedPtr<SWidget> SCustomGitFileListPanel::OnOpenContextMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    if (CurrentViewMode == EGitViewMode::LocalChanges)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("StageSelected", "Stage Selected Files"),
            LOCTEXT("StageSelectedTooltip", "Add selected files to staging area"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextStageSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("StashSelected", "Stash Selected Files"),
            LOCTEXT("StashSelectedTooltip", "Stash selected files for later"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextStashSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("DiscardChanges", "Discard Changes"),
            LOCTEXT("DiscardChangesTooltip", "Discard changes to selected files (git checkout)"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextDiscardChanges)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("LockSelected", "Lock Selected Files"),
            LOCTEXT("LockSelectedTooltip", "Lock selected files with Git LFS"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextLockSelected)));
    }
    else if (CurrentViewMode == EGitViewMode::StagedChanges)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("UnstageSelected", "Unstage Selected Files"),
            LOCTEXT("UnstageSelectedTooltip", "Remove selected files from staging area"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextUnstageSelected)));

        MenuBuilder.AddMenuEntry(
            LOCTEXT("LockSelectedStaged", "Lock Selected Files"),
            LOCTEXT("LockSelectedStagedTooltip", "Lock selected files with Git LFS"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextLockSelected)));
    }
    else if (CurrentViewMode == EGitViewMode::LockedFiles)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("ReleaseLock", "Release Lock"),
            LOCTEXT("ReleaseLockTooltip", "Release the LFS lock on selected files"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SCustomGitFileListPanel::OnContextUnlockSelected)));
    }
    else
    {
        return nullptr;
    }

    return MenuBuilder.MakeWidget();
}

void SCustomGitFileListPanel::OnContextStageSelected()
{
    TArray<FString> FilesToStage;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            FilesToStage.Add(Item->Filename);
        }
    }

    if (FilesToStage.Num() > 0 && OnStageFilesDelegate.IsBound())
    {
        OnStageFilesDelegate.Execute(FilesToStage);
    }
}

void SCustomGitFileListPanel::OnContextUnstageSelected()
{
    TArray<FString> FilesToUnstage;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            FilesToUnstage.Add(Item->Filename);
        }
    }

    if (FilesToUnstage.Num() > 0 && OnUnstageFilesDelegate.IsBound())
    {
        OnUnstageFilesDelegate.Execute(FilesToUnstage);
    }
}

void SCustomGitFileListPanel::OnContextLockSelected()
{
    TArray<FString> FilesToLock;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            FilesToLock.Add(Item->Filename);
        }
    }

    if (FilesToLock.Num() > 0 && OnLockFilesDelegate.IsBound())
    {
        OnLockFilesDelegate.Execute(FilesToLock);
    }
}

void SCustomGitFileListPanel::OnContextUnlockSelected()
{
    TArray<FString> FilesToUnlock;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            FilesToUnlock.Add(Item->Filename);
        }
    }

    if (FilesToUnlock.Num() > 0 && OnUnlockFilesDelegate.IsBound())
    {
        OnUnlockFilesDelegate.Execute(FilesToUnlock);
    }
}

void SCustomGitFileListPanel::OnContextDiscardChanges()
{
    TArray<TSharedPtr<FGitFileStatus>> SelectedItems;
    if (ListView.IsValid())
    {
        SelectedItems = ListView->GetSelectedItems();
    }

    TArray<FString> TrackedFiles;
    TArray<FString> UntrackedFiles;

    for (const auto& Item : SelectedItems)
    {
        // Check if it's an untracked file
        if (Item->Status == TEXT("Untracked") || Item->Status.Contains(TEXT("??")))
        {
            UntrackedFiles.Add(Item->Filename);
        }
        else
        {
            TrackedFiles.Add(Item->Filename);
        }
    }

    int32 TotalFiles = TrackedFiles.Num() + UntrackedFiles.Num();
    if (TotalFiles > 0)
    {
        // Build confirmation message
        FString MessageText;
        if (UntrackedFiles.Num() > 0 && TrackedFiles.Num() > 0)
        {
            MessageText = FString::Printf(TEXT("Are you sure you want to discard changes to %d file(s)?\n\n%d tracked file(s) will be reverted.\n%d untracked file(s) will be DELETED.\n\nThis cannot be undone."),
                TotalFiles, TrackedFiles.Num(), UntrackedFiles.Num());
        }
        else if (UntrackedFiles.Num() > 0)
        {
            MessageText = FString::Printf(TEXT("Are you sure you want to DELETE %d untracked file(s)?\n\nThis cannot be undone."),
                UntrackedFiles.Num());
        }
        else
        {
            MessageText = FString::Printf(TEXT("Are you sure you want to discard changes to %d file(s)?\n\nThis cannot be undone."),
                TrackedFiles.Num());
        }

        EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(MessageText));
        if (Result == EAppReturnType::Yes)
        {
            TArray<FString> AllFilesToDiscard;
            AllFilesToDiscard.Append(TrackedFiles);
            AllFilesToDiscard.Append(UntrackedFiles);

            if (OnDiscardFilesDelegate.IsBound())
            {
                OnDiscardFilesDelegate.Execute(AllFilesToDiscard);
            }
        }
    }
}

void SCustomGitFileListPanel::OnContextStashSelected()
{
    TArray<FString> FilesToStash;
    if (ListView.IsValid())
    {
        TArray<TSharedPtr<FGitFileStatus>> SelectedItems = ListView->GetSelectedItems();
        for (const auto& Item : SelectedItems)
        {
            FilesToStash.Add(Item->Filename);
        }
    }

    if (FilesToStash.Num() > 0 && OnStashFilesDelegate.IsBound())
    {
        OnStashFilesDelegate.Execute(FilesToStash);
    }
}

void SCustomGitFileListPanel::RefreshList()
{
    if (ListView.IsValid())
    {
        ListView->RequestListRefresh();
    }
}

void SCustomGitFileListPanel::SetFileList(TArray<TSharedPtr<FGitFileStatus>>* InFileList)
{
    CurrentFileList = InFileList;
    if (ListView.IsValid())
    {
        ListView->SetItemsSource(CurrentFileList);
        ListView->RequestListRefresh();
    }
}

void SCustomGitFileListPanel::SetViewMode(EGitViewMode InViewMode)
{
    CurrentViewMode = InViewMode;
}

#undef LOCTEXT_NAMESPACE
