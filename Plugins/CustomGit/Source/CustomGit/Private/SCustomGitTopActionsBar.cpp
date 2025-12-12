#include "SCustomGitTopActionsBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SCustomGitTopActionsBar"

void SCustomGitTopActionsBar::Construct(const FArguments& InArgs)
{
    CurrentBranchName = InArgs._CurrentBranchName;
    OnSyncDelegate = InArgs._OnSyncClicked;
    OnResetDelegate = InArgs._OnResetClicked;

    // Build button text with branch name
    FString SyncButtonText;
    if (!CurrentBranchName.IsEmpty())
    {
        SyncButtonText = FString::Printf(TEXT("Sync (Push/Pull) - %s"), *CurrentBranchName);
    }
    else
    {
        SyncButtonText = TEXT("Sync (Push/Pull)");
    }

    ChildSlot
        [SNew(SHorizontalBox)

            // Sync button
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2)
                [SAssignNew(SyncButton, SButton)
                    .Text(FText::FromString(SyncButtonText))
                    .OnClicked(this, &SCustomGitTopActionsBar::OnSyncButtonClicked)]

            // Reset button
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2)
                [SNew(SButton)
                    .Text(LOCTEXT("ResetBtn", "Reset Hard to Origin"))
                    .OnClicked(this, &SCustomGitTopActionsBar::OnResetButtonClicked)]
        ];
}

FReply SCustomGitTopActionsBar::OnSyncButtonClicked()
{
    if (OnSyncDelegate.IsBound())
    {
        OnSyncDelegate.Execute();
    }
    return FReply::Handled();
}

FReply SCustomGitTopActionsBar::OnResetButtonClicked()
{
    if (OnResetDelegate.IsBound())
    {
        OnResetDelegate.Execute();
    }
    return FReply::Handled();
}

void SCustomGitTopActionsBar::SetBranchName(const FString& InBranchName)
{
    CurrentBranchName = InBranchName;

    if (SyncButton.IsValid() && !CurrentBranchName.IsEmpty())
    {
        FString SyncText = FString::Printf(TEXT("Sync (Push/Pull) - %s"), *CurrentBranchName);
        SyncButton->SetContent(
            SNew(STextBlock)
                .Text(FText::FromString(SyncText)));
    }
}

#undef LOCTEXT_NAMESPACE
