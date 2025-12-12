#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// Delegate for commit requested event
DECLARE_DELEGATE_OneParam(FOnCommitRequested, const FString& /* CommitMessage */);

/**
 * SCustomGitCommitPanel - Commit message input and author info display
 * Provides UI for entering commit messages and displaying git user information
 */
class SCustomGitCommitPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomGitCommitPanel)
        {}
        SLATE_ARGUMENT(FString, UserName)
        SLATE_ARGUMENT(FString, UserEmail)
        SLATE_EVENT(FOnCommitRequested, OnCommit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Update user info display
    void SetUserInfo(const FString& InName, const FString& InEmail);

    // Get current commit message
    FString GetCommitMessage() const;

    // Clear commit message
    void ClearCommitMessage();

private:
    // UI Callbacks
    FReply OnCommitClicked();

    // Delegates
    FOnCommitRequested OnCommitDelegate;

    // Data
    FString UserName;
    FString UserEmail;

    // Widgets
    TSharedPtr<class STextBlock> AuthorTextBlock;
    TSharedPtr<class SMultiLineEditableTextBox> CommitMessageInput;
};
