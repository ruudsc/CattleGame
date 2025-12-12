#include "SCustomGitCommitPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "EditorStyleSet.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SCustomGitCommitPanel"

void SCustomGitCommitPanel::Construct(const FArguments& InArgs)
{
    UserName = InArgs._UserName;
    UserEmail = InArgs._UserEmail;
    OnCommitDelegate = InArgs._OnCommit;

    ChildSlot
        [SNew(SVerticalBox)

            // Author info
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [SAssignNew(AuthorTextBlock, STextBlock)
                    .Text(FText::FromString(
                        FString::Printf(TEXT("Author: %s <%s>"), *UserName, *UserEmail)))]

            // Commit message label
            + SVerticalBox::Slot()
                .AutoHeight()
                [SNew(STextBlock)
                    .Text(LOCTEXT("CommitMsg", "Commit Message:"))]

            // Commit message input
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [SAssignNew(CommitMessageInput, SMultiLineEditableTextBox)
                    .HintText(LOCTEXT("CommitHint", "Enter commit message..."))]

            // Commit button
            + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                .Padding(0, 2)
                [SNew(SButton)
                    .Text(LOCTEXT("CommitBtn", "Commit Changes"))
                    .OnClicked(this, &SCustomGitCommitPanel::OnCommitClicked)]
        ];
}

FReply SCustomGitCommitPanel::OnCommitClicked()
{
    FString Message = CommitMessageInput->GetText().ToString();
    if (Message.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoCommitMessage", "Please enter a commit message."));
        return FReply::Handled();
    }

    // Fire the delegate
    if (OnCommitDelegate.IsBound())
    {
        OnCommitDelegate.Execute(Message);
    }

    return FReply::Handled();
}

void SCustomGitCommitPanel::SetUserInfo(const FString& InName, const FString& InEmail)
{
    UserName = InName;
    UserEmail = InEmail;

    if (AuthorTextBlock.IsValid())
    {
        FString AuthorText = FString::Printf(TEXT("Author: %s <%s>"), *UserName, *UserEmail);
        AuthorTextBlock->SetText(FText::FromString(AuthorText));
    }
}

FString SCustomGitCommitPanel::GetCommitMessage() const
{
    if (CommitMessageInput.IsValid())
    {
        return CommitMessageInput->GetText().ToString();
    }
    return FString();
}

void SCustomGitCommitPanel::ClearCommitMessage()
{
    if (CommitMessageInput.IsValid())
    {
        CommitMessageInput->SetText(FText::GetEmpty());
    }
}

#undef LOCTEXT_NAMESPACE
