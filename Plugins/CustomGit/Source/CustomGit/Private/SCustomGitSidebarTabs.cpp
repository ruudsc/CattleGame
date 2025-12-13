#include "SCustomGitSidebarTabs.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "SCustomGitSidebarTabs"

void SCustomGitSidebarTabs::Construct(const FArguments &InArgs)
{
    StashList = InArgs._StashList;
    CommitHistoryList = InArgs._CommitHistoryList;
    CommandHistoryList = InArgs._CommandHistoryList;
    OnPopStashDelegate = InArgs._OnPopStash;
    OnDropStashDelegate = InArgs._OnDropStash;
    OnRefreshDelegate = InArgs._OnRefreshRequested;
    OnClearCommandHistoryDelegate = InArgs._OnClearCommandHistory;
    CurrentSidebarTab = ESidebarTab::CommitHistory;

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
                                   .Text(LOCTEXT("TabCommits", "Commits"))
                                   .OnClicked(this, &SCustomGitSidebarTabs::OnTabCommitsClicked)]

                    + SHorizontalBox::Slot()
                          .FillWidth(1.0f)
                          .Padding(1)
                              [SNew(SButton)
                                   .Text(LOCTEXT("TabCmdHist", "Cmds"))
                                   .OnClicked(this, &SCustomGitSidebarTabs::OnTabCommandsClicked)]

                    + SHorizontalBox::Slot()
                          .FillWidth(1.0f)
                          .Padding(1)
                              [SNew(SButton)
                                   .Text(LOCTEXT("TabStashes", "Stashes"))
                                   .OnClicked(this, &SCustomGitSidebarTabs::OnTabStashesClicked)]]

         // Content area - switches between list view and text box
         + SVerticalBox::Slot()
               .FillHeight(1.0f)
                   [SAssignNew(ContentSwitcher, SWidgetSwitcher)
                        .WidgetIndex(0)

                    // Slot 0: List view for Stashes and Commit History
                    + SWidgetSwitcher::Slot()
                          [SAssignNew(SidebarListView, SListView<TSharedPtr<FString>>)
                               .ListItemsSource(CommitHistoryList ? CommitHistoryList : nullptr)
                               .OnGenerateRow(this, &SCustomGitSidebarTabs::OnGenerateSidebarRow)]

                    // Slot 1: Text box for Command History with context menu button
                    + SWidgetSwitcher::Slot()
                          [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(2)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("ClearCmdHistory", "Clear")).ToolTipText(LOCTEXT("ClearCmdHistoryTip", "Clear command history")).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                    {
                                                if (OnClearCommandHistoryDelegate.IsBound())
                                                {
                                                    OnClearCommandHistoryDelegate.Execute();
                                                }
                                                return FReply::Handled(); })]] +
                           SVerticalBox::Slot()
                               .FillHeight(1.0f)
                                   [SNew(SBorder)
                                        .Padding(2)
                                            [SAssignNew(CommandHistoryTextBox, SMultiLineEditableTextBox)
                                                 .IsReadOnly(true)
                                                 .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))]]]]];
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

    if (!ContentSwitcher.IsValid())
    {
        return;
    }

    if (NewTab == ESidebarTab::CommandHistory)
    {
        // Switch to text box view
        ContentSwitcher->SetActiveWidgetIndex(1);
        RefreshCommandHistory();
    }
    else
    {
        // Switch to list view
        ContentSwitcher->SetActiveWidgetIndex(0);

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
        default:
            break;
        }

        SidebarListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SCustomGitSidebarTabs::OnGenerateSidebarRow(TSharedPtr<FString> InItem,
                                                                  const TSharedRef<STableViewBase> &OwnerTable)
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
                            return FReply::Handled(); })]

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
                            return FReply::Handled(); })]];
    }
    else if (CurrentSidebarTab == ESidebarTab::CommandHistory)
    {
        // Read-only editable text box for command history (allows copying)
        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox)

             + SHorizontalBox::Slot()
                   .FillWidth(1.0f)
                   .Padding(2)
                       [SNew(SEditableTextBox)
                            .Text(FText::FromString(*InItem))
                            .IsReadOnly(true)
                            .Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))]];
    }
    else
    {
        // Simple text display for commit history
        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
            [SNew(SHorizontalBox)

             + SHorizontalBox::Slot()
                   .FillWidth(1.0f)
                   .Padding(5)
                       [SNew(STextBlock)
                            .Text(FText::FromString(*InItem))
                            .ToolTipText(FText::FromString(*InItem))]];
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
    if (CommandHistoryTextBox.IsValid() && CommandHistoryList)
    {
        // Build a single text string from all command history entries
        FString CombinedText;
        for (int32 i = 0; i < CommandHistoryList->Num(); i++)
        {
            if (i > 0)
            {
                CombinedText += TEXT("\n");
            }
            CombinedText += *(*CommandHistoryList)[i];
        }
        CommandHistoryTextBox->SetText(FText::FromString(CombinedText));
    }
}

#undef LOCTEXT_NAMESPACE
