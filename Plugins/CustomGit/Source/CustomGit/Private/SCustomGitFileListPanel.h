#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Input/DragAndDrop.h"
#include "SCustomGitViewModeSelector.h"

// Forward declarations
struct FGitFileStatus;

// Delegate for file operations
DECLARE_DELEGATE_OneParam(FOnFilesChanged, const TArray<FString>& /* FileList */);

// Custom DragDropOperation to carry file information
class FGitFileDragDropOp : public FDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FGitFileDragDropOp, FDragDropOperation)

    TArray<FString> FilesToStage;

    static TSharedRef<FGitFileDragDropOp> New(const TArray<FString>& InFiles)
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
                    .Text(FText::Format(NSLOCTEXT("SCustomGitFileListPanel", "DragFiles", "Stage {0} files"),
                        FText::AsNumber(FilesToStage.Num())))
                    .ColorAndOpacity(FLinearColor::White)];

        CursorDecoratorWindow = SNew(SWindow)
            .Style(FAppStyle::Get(), "Cursor.Window")
            .Content()
            [Decorator.ToSharedRef()];
    }
};

/**
 * SCustomGitFileListPanel - File list display with drag-drop and context menus
 * Shows files from git status with abilities to stage, unstage, lock, discard, and stash
 */
class SCustomGitFileListPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitFileListPanel)
        {}
        SLATE_ARGUMENT(TArray<TSharedPtr<FGitFileStatus>>*, FileList)
        SLATE_ARGUMENT(FString, ViewModeName)
        SLATE_ARGUMENT(EGitViewMode, ViewMode)
        SLATE_EVENT(FOnFilesChanged, OnStageFiles)
        SLATE_EVENT(FOnFilesChanged, OnUnstageFiles)
        SLATE_EVENT(FOnFilesChanged, OnLockFiles)
        SLATE_EVENT(FOnFilesChanged, OnUnlockFiles)
        SLATE_EVENT(FOnFilesChanged, OnDiscardFiles)
        SLATE_EVENT(FOnFilesChanged, OnStashFiles)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Refresh the list display
    void RefreshList();

    // Set file list source
    void SetFileList(TArray<TSharedPtr<FGitFileStatus>>* InFileList);

    // Update view mode
    void SetViewMode(EGitViewMode InViewMode);

private:
    // UI Callbacks
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FGitFileStatus> InItem,
        const TSharedRef<STableViewBase>& OwnerTable);

    TSharedPtr<SWidget> OnOpenContextMenu();

    // Context menu actions
    void OnContextStageSelected();
    void OnContextUnstageSelected();
    void OnContextLockSelected();
    void OnContextUnlockSelected();
    void OnContextDiscardChanges();
    void OnContextStashSelected();

    // Drag and Drop
    FReply OnListDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

    // Data
    TArray<TSharedPtr<FGitFileStatus>>* CurrentFileList = nullptr;
    EGitViewMode CurrentViewMode = EGitViewMode::LocalChanges;

    // Delegates
    FOnFilesChanged OnStageFilesDelegate;
    FOnFilesChanged OnUnstageFilesDelegate;
    FOnFilesChanged OnLockFilesDelegate;
    FOnFilesChanged OnUnlockFilesDelegate;
    FOnFilesChanged OnDiscardFilesDelegate;
    FOnFilesChanged OnStashFilesDelegate;

    // Widgets
    TSharedPtr<SListView<TSharedPtr<FGitFileStatus>>> ListView;
    TSharedPtr<class SHeaderRow> HeaderRow;
};
