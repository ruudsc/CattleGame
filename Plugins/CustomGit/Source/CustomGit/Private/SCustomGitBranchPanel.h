#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "CustomGitOperations.h"

// Delegate for branch operations
DECLARE_DELEGATE_OneParam(FOnBranchAction, const FString& /* BranchName */);

/**
 * SCustomGitBranchPanel - Branch management panel
 * Displays branch list and provides operations like switch, create, delete, push, reset
 */
class SCustomGitBranchPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitBranchPanel)
        {}
        SLATE_ARGUMENT(TArray<TSharedPtr<FGitBranchInfo>>*, BranchList)
        SLATE_ARGUMENT(FString, CurrentBranchName)
        SLATE_EVENT(FOnBranchAction, OnSwitchBranch)
        SLATE_EVENT(FOnBranchAction, OnCreateBranch)
        SLATE_EVENT(FOnBranchAction, OnDeleteBranch)
        SLATE_EVENT(FOnBranchAction, OnPushBranch)
        SLATE_EVENT(FOnBranchAction, OnResetBranch)
        SLATE_EVENT(FOnBranchAction, OnRefreshRequested)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Update branch list display
    void RefreshBranchList();

    // Set current branch name
    void SetCurrentBranchName(const FString& InBranchName);

private:
    // UI Callbacks
    TSharedRef<ITableRow> OnGenerateBranchRow(TSharedPtr<FGitBranchInfo> InItem,
        const TSharedRef<STableViewBase>& OwnerTable);

    TSharedPtr<SWidget> OnOpenBranchContextMenu();

    // Context menu actions - create branch
    FReply OnCreateBranchClicked();

    // Data
    TArray<TSharedPtr<FGitBranchInfo>>* BranchList = nullptr;
    FString CurrentBranchName;

    // Delegates
    FOnBranchAction OnSwitchBranchDelegate;
    FOnBranchAction OnCreateBranchDelegate;
    FOnBranchAction OnDeleteBranchDelegate;
    FOnBranchAction OnPushBranchDelegate;
    FOnBranchAction OnResetBranchDelegate;
    FOnBranchAction OnRefreshDelegate;

    // Widgets
    TSharedPtr<SListView<TSharedPtr<FGitBranchInfo>>> BranchListView;
    TSharedPtr<class SEditableTextBox> NewBranchNameInput;
};
