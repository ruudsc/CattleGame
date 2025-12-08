#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Input/DragAndDrop.h"

enum class EGitViewMode
{
    LocalChanges,
    StagedChanges,
    LockedFiles,
    History
};

enum class ESidebarTab
{
    Stashes,
    CommitHistory,
    CommandHistory
};

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

#include "EditorStyleSet.h"

// Custom DragDropOperation to carry file information
class FGitFileDragDropOp : public FDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FGitFileDragDropOp, FDragDropOperation)

    TArray<FString> FilesToStage;

    static TSharedRef<FGitFileDragDropOp> New(const TArray<FString> &InFiles)
    {
        TSharedRef<FGitFileDragDropOp> Operation = MakeShareable(new FGitFileDragDropOp);
        Operation->FilesToStage = InFiles;
        Operation->Construct();
        return Operation;
    }

    virtual void Construct() override
    {
        FDragDropOperation::Construct();

        TSharedPtr<SWidget> Decorator =
            SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("Menu.Background"))
                .Padding(FMargin(5.0f))
                    [SNew(STextBlock)
                         .Text(FText::Format(NSLOCTEXT("SCustomGitWindow", "DragFiles", "Stage {0} files"), FText::AsNumber(FilesToStage.Num())))
                         .ColorAndOpacity(FLinearColor::White)];

        CursorDecoratorWindow = SNew(SWindow)
                                    .Style(FAppStyle::Get(), "Cursor.Window")
                                    .Content()
                                        [Decorator.ToSharedRef()];
    }
};

class SCustomGitWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs);

    // Refresh the UI
    void RefreshStatus();

private:
    // UI Callbacks
    FReply OnCommitClicked();
    FReply OnRefreshClicked();
    FReply OnCreateBranchClicked();
    FReply OnSwitchBranchClicked(); // logic might be via ComboBox OnSelectionChanged

    // Sidebar Callbacks
    FReply OnViewLocalClicked();
    FReply OnViewStagedClicked();
    FReply OnViewLockedClicked();
    FReply OnViewHistoryClicked();

    // Drag and Drop (Source)
    FReply OnListDragDetected(const FGeometry &MyGeometry, const FPointerEvent &MouseEvent);

    // Drag and Drop (Target - Staged Button)
    FReply OnStagedDrop(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent);
    void OnStagedDragEnter(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent);
    void OnStagedDragLeave(const FDragDropEvent &DragDropEvent);

    // Drag and Drop (Target - Local Button for unstaging)
    FReply OnLocalDrop(const FGeometry &MyGeometry, const FDragDropEvent &DragDropEvent);

    // Context Menu
    TSharedPtr<SWidget> OnOpenContextMenu();
    TSharedPtr<SWidget> OnOpenBranchContextMenu();
    void OnContextStageSelected();
    void OnContextUnstageSelected();
    void OnContextReleaseLock();
    void OnContextStashSelected();
    void OnContextDiscardChanges();
    void OnContextLockSelected();

    // Action Buttons
    FReply OnSyncClicked();
    FReply OnResetHardClicked();

    void OnBranchSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

    // Branch List Generation
    TSharedRef<ITableRow> OnGenerateBranchRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase> &OwnerTable);

    // Stash/History List
    void UpdateStashList();

    // List View Generation
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FGitFileStatus> InItem, const TSharedRef<STableViewBase> &OwnerTable);

    // Data Source
    TArray<TSharedPtr<FGitFileStatus>> LocalFileList;
    TArray<TSharedPtr<FGitFileStatus>> StagedFileList;
    TArray<TSharedPtr<FGitFileStatus>> LockedFileList;
    TArray<TSharedPtr<FGitFileStatus>> HistoryFileList;
    TArray<TSharedPtr<FGitFileStatus>> *CurrentFileList = &LocalFileList; // Pointer to current active list

    TArray<TSharedPtr<FString>> BranchList;
    TArray<TSharedPtr<FString>> StashList;
    TArray<TSharedPtr<FString>> CommitHistoryList;
    TArray<TSharedPtr<FString>> CommandHistoryList;
    FString CurrentBranchName;
    FString CurrentUserName;
    FString CurrentUserEmail;

    // View State
    EGitViewMode CurrentViewMode = EGitViewMode::LocalChanges;
    ESidebarTab CurrentSidebarTab = ESidebarTab::Stashes;

    // Widgets
    TSharedPtr<SListView<TSharedPtr<FGitFileStatus>>> ListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> BranchListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> StashListView;
    TSharedPtr<SListView<TSharedPtr<FString>>> SidebarListView;
    TSharedPtr<class SWidgetSwitcher> SidebarTabSwitcher;

    TSharedPtr<class SMultiLineEditableTextBox> CommitMessageInput;
    TSharedPtr<class SEditableTextBox> NewBranchNameInput;
    TSharedPtr<class SVerticalBox> CommitBox;
    TSharedPtr<class STextBlock> AuthorTextBlock;

    // Commands implementation details
    void UpdateBranchList();
    void UpdateUserInfo();
    void UpdateCommitHistory();
    void UpdateCommandHistory();
    void OnSidebarTabChanged(ESidebarTab NewTab);
    TSharedRef<ITableRow> OnGenerateSidebarRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase> &OwnerTable);
    TSharedPtr<FString> CurrentSelectedBranch;
};
