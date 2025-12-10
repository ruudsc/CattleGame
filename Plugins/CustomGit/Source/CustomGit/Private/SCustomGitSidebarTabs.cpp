#include "SCustomGitSidebarTabs.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SCustomGitSidebarTabs"

void SCustomGitSidebarTabs::Construct(const FArguments& InArgs)
{
    StashList = InArgs._StashList;
    CommitHistoryList = InArgs._CommitHistoryList;
    CommandHistoryList = InArgs._CommandHistoryList;
    OnPopStashDelegate = InArgs._OnPopStash;
    OnDropStashDelegate = InArgs._OnDropStash;
    OnRefreshDelegate = InArgs._OnRefreshRequested;
    CurrentSidebarTab = ESidebarTab::Stashes;

    ChildSlot
        [SNew(SVerticalBox)

            // Tab buttons
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2)
                [SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(1)
                        [SNew(SButton)
                            .Text(LOCTEXT("TabStashes", "Stashes"))
                            .OnClicked(this, &SCustomGitSidebarTabs::OnTabStashesClicked)]

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(1)
                        [SNew(SButton)
                            .Text(LOCTEXT("TabCommits", "Commits"))
                            .OnClicked(this, &SCustomGitSidebarTabs::OnTabCommitsClicked)]

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(1)
                        [SNew(SButton)
                            .Text(LOCTEXT("TabCmdHist", "Cmds"))
                            .OnClicked(this, &SCustomGitSidebarTabs::OnTabCommandsClicked)]
                ]

            // List view showing current tab content
            + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [SAssignNew(SidebarListView, SListView<TSharedPtr<FString>>)
                    .ListItemsSource(StashList ? StashList : nullptr)
                    .OnGenerateRow(this, &SCustomGitSidebarTabs::OnGenerateSidebarRow)]
        ];
}

FReply SCustomGitSidebarTabs::OnTabStashesClicked()
{
    SetActiveTab(ESidebarTab::Stashes);
    return FReply::Handled();
}

FReply SCustomGitSidebarTabs::OnTabCommitsClicked()
{
    SetActiveTab(ESidebarTab::CommitHistory);
    return FReply::Handled();
}

FReply SCustomGitSidebarTabs::OnTabCommandsClicked()
{
    SetActiveTab(ESidebarTab::CommandHistory);
    return FReply::Handled();
}

void SCustomGitSidebarTabs::SetActiveTab(ESidebarTab NewTab)
{
    CurrentSidebarTab = NewTab;

    if (!SidebarListView.IsValid())
    {
        return;
    }

    switch (NewTab)
    {
    case ESidebarTab::Stashes:
        if (StashList)
        {
            SidebarListView->SetItemsSource(StashList);
        }
        break;
    case ESidebarTab::CommitHistory:
        if (CommitHistoryList)
        {
            SidebarListView->SetItemsSource(CommitHistoryList);
        }
        break;
    case ESidebarTab::CommandHistory:
        if (CommandHistoryList)
        {
            SidebarListView->SetItemsSource(CommandHistoryList);
        }
        break;
    }

    SidebarListView->RequestListRefresh();
}

TSharedRef<ITableRow> SCustomGitSidebarTabs::OnGenerateSidebarRow(TSharedPtr<FString> InItem,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    // Different display based on current tab
    if (CurrentSidebarTab == ESidebarTab::Stashes)
    {
        // Parse stash entry: "stash@{0}: WIP on branch: message"
        FString StashRef;
        FString StashMessage = *InItem;

        int32 ColonIndex;
        if (InItem->FindChar(TEXT(':'), ColonIndex))
        {
            StashRef = InItem->Left(ColonIndex);
            StashMessage = InItem->Mid(ColonIndex + 1).TrimStart();
        }

        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox)

                // Stash message
                + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5)
                    .VAlign(VAlign_Center)
                    [SNew(STextBlock)
                        .Text(FText::FromString(StashMessage))
                        .ToolTipText(FText::FromString(*InItem))]

                // Pop button
                + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(2)
                    [SNew(SButton)
                        .Text(LOCTEXT("PopStash", "Pop"))
                        .OnClicked_Lambda([this, StashRef]()
                        {
                            if (OnPopStashDelegate.IsBound())
                            {
                                OnPopStashDelegate.Execute(StashRef);
                            }
                            return FReply::Handled();
                        })]

                // Drop button
                + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(2)
                    [SNew(SButton)
                        .Text(LOCTEXT("DropStash", "Drop"))
                        .OnClicked_Lambda([this, StashRef]()
                        {
                            if (OnDropStashDelegate.IsBound())
                            {
                                OnDropStashDelegate.Execute(StashRef);
                            }
                            return FReply::Handled();
                        })]
            ];
    }
    else
    {
        // Simple text display for commit history and command history
        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5)
                    [SNew(STextBlock)
                        .Text(FText::FromString(*InItem))
                        .ToolTipText(FText::FromString(*InItem))]
            ];
    }
}

void SCustomGitSidebarTabs::RefreshStashes()
{
    if (SidebarListView.IsValid() && CurrentSidebarTab == ESidebarTab::Stashes)
    {
        SidebarListView->RequestListRefresh();
    }
}

void SCustomGitSidebarTabs::RefreshCommitHistory()
{
    if (SidebarListView.IsValid() && CurrentSidebarTab == ESidebarTab::CommitHistory)
    {
        SidebarListView->RequestListRefresh();
    }
}

void SCustomGitSidebarTabs::RefreshCommandHistory()
{
    if (SidebarListView.IsValid() && CurrentSidebarTab == ESidebarTab::CommandHistory)
    {
        SidebarListView->RequestListRefresh();
    }
}

#undef LOCTEXT_NAMESPACE
