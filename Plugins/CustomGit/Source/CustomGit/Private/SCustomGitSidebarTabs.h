#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

enum class ESidebarTab
{
    Stashes,
    CommitHistory,
    CommandHistory
};

// Delegate for stash operations
DECLARE_DELEGATE_OneParam(FOnStashAction, const FString& /* StashRef */);

/**
 * SCustomGitSidebarTabs - Sidebar tabbed panel for Stashes, Commit History, Command History
 * Provides UI for viewing and managing stashes and displaying git history
 */
class SCustomGitSidebarTabs : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitSidebarTabs)
        {}
        SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, StashList)
        SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, CommitHistoryList)
        SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, CommandHistoryList)
        SLATE_EVENT(FOnStashAction, OnPopStash)
        SLATE_EVENT(FOnStashAction, OnDropStash)
        SLATE_EVENT(FSimpleDelegate, OnRefreshRequested)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Update lists
    void RefreshStashes();
    void RefreshCommitHistory();
    void RefreshCommandHistory();

    // Change active tab
    void SetActiveTab(ESidebarTab NewTab);

private:
    // UI Callbacks
    FReply OnTabStashesClicked();
    FReply OnTabCommitsClicked();
    FReply OnTabCommandsClicked();

    TSharedRef<ITableRow> OnGenerateSidebarRow(TSharedPtr<FString> InItem,
        const TSharedRef<STableViewBase>& OwnerTable);

    // Data
    TArray<TSharedPtr<FString>>* StashList = nullptr;
    TArray<TSharedPtr<FString>>* CommitHistoryList = nullptr;
    TArray<TSharedPtr<FString>>* CommandHistoryList = nullptr;
    ESidebarTab CurrentSidebarTab = ESidebarTab::Stashes;

    // Delegates
    FOnStashAction OnPopStashDelegate;
    FOnStashAction OnDropStashDelegate;
    FSimpleDelegate OnRefreshDelegate;

    // Widgets
    TSharedPtr<SListView<TSharedPtr<FString>>> SidebarListView;
};
