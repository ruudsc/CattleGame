#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// Delegate for action buttons
DECLARE_DELEGATE(FSimpleDelegate);

/**
 * SCustomGitTopActionsBar - Top action buttons bar
 * Provides access to high-level git operations like Sync and Reset
 */
class SCustomGitTopActionsBar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitTopActionsBar)
        {}
        SLATE_ARGUMENT(FString, CurrentBranchName)
        SLATE_EVENT(FSimpleDelegate, OnSyncClicked)
        SLATE_EVENT(FSimpleDelegate, OnResetClicked)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Update branch name display on sync button
    void SetBranchName(const FString& InBranchName);

private:
    // UI Callbacks
    FReply OnSyncButtonClicked();
    FReply OnResetButtonClicked();

    // Data
    FString CurrentBranchName;

    // Delegates
    FSimpleDelegate OnSyncDelegate;
    FSimpleDelegate OnResetDelegate;

    // Widgets
    TSharedPtr<class SButton> SyncButton;
};
