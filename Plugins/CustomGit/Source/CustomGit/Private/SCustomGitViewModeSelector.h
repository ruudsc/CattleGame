#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

enum class EGitViewMode
{
    LocalChanges,
    StagedChanges,
    LockedFiles,
    History
};

// Struct for view mode list items
struct FViewModeItem
{
    FString Name;
    EGitViewMode ViewMode;
};

// Delegate for view mode changed event
DECLARE_DELEGATE_OneParam(FOnViewModeChanged, EGitViewMode /* NewViewMode */);

/**
 * SCustomGitViewModeSelector - Sidebar view mode selector
 * Allows users to switch between Local Changes, Staged Changes, and Locked Files views
 */
class SCustomGitViewModeSelector : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitViewModeSelector)
        {}
        SLATE_ARGUMENT(EGitViewMode, InitialViewMode)
        SLATE_EVENT(FOnViewModeChanged, OnViewModeChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Set current view mode (external)
    void SetViewMode(EGitViewMode NewMode, bool bNotify = true);

    // Get current view mode
    EGitViewMode GetCurrentViewMode() const { return CurrentViewMode; }

private:
    // UI Callbacks
    TSharedRef<ITableRow> OnGenerateViewModeRow(TSharedPtr<FViewModeItem> InItem,
        const TSharedRef<STableViewBase>& OwnerTable);

    void OnViewModeSelectionChanged(TSharedPtr<FViewModeItem> SelectedItem,
        ESelectInfo::Type SelectInfo);

    // Data
    TArray<TSharedPtr<FViewModeItem>> ViewModeList;
    EGitViewMode CurrentViewMode = EGitViewMode::LocalChanges;

    // Delegates
    FOnViewModeChanged OnViewModeChangedDelegate;

    // Widgets
    TSharedPtr<SListView<TSharedPtr<FViewModeItem>>> ViewModeListView;
};
