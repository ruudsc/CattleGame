#include "SCustomGitBranchPanel.h"
#include "CustomGitOperations.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SCustomGitBranchPanel"

void SCustomGitBranchPanel::Construct(const FArguments& InArgs)
{
    BranchList = InArgs._BranchList;
    CurrentBranchName = InArgs._CurrentBranchName;
    OnSwitchBranchDelegate = InArgs._OnSwitchBranch;
    OnCreateBranchDelegate = InArgs._OnCreateBranch;
    OnDeleteBranchDelegate = InArgs._OnDeleteBranch;
    OnPushBranchDelegate = InArgs._OnPushBranch;
    OnResetBranchDelegate = InArgs._OnResetBranch;
    OnRefreshDelegate = InArgs._OnRefreshRequested;

    ChildSlot
        [SNew(SVerticalBox)

            // Header
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2)
                [SNew(STextBlock)
                    .Text(LOCTEXT("BranchesHeader", "Branches"))
                    .Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))]

            // Branch list
            + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [SAssignNew(BranchListView, SListView<TSharedPtr<FGitBranchInfo>>)
                    .ListItemsSource(BranchList ? BranchList : nullptr)
                    .OnGenerateRow(this, &SCustomGitBranchPanel::OnGenerateBranchRow)
                    .OnContextMenuOpening(this, &SCustomGitBranchPanel::OnOpenBranchContextMenu)]

            // Create branch section
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2, 5)
                [SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [SAssignNew(NewBranchNameInput, SEditableTextBox)
                            .HintText(LOCTEXT("NewBranchHint", "New Branch Name"))]

                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2, 0, 0, 0)
                        [SNew(SButton)
                            .Text(LOCTEXT("Create", "+"))
                            .OnClicked(this, &SCustomGitBranchPanel::OnCreateBranchClicked)]
                ]
        ];
}

TSharedRef<ITableRow> SCustomGitBranchPanel::OnGenerateBranchRow(TSharedPtr<FGitBranchInfo> InItem,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    // Build status suffix text and color
    FString StatusSuffix;
    FSlateColor StatusColor = FSlateColor(FLinearColor::Gray);

    if (InItem->bIsUpstreamGone)
    {
        StatusSuffix = TEXT(" (upstream gone)");
        StatusColor = FSlateColor(FLinearColor::Red);
    }
    else if (InItem->bIsLocal)
    {
        StatusSuffix = TEXT(" (local)");
        StatusColor = FSlateColor(FLinearColor::Gray);
    }

    return SNew(STableRow<TSharedPtr<FGitBranchInfo>>, OwnerTable)
        [SNew(SHorizontalBox)

            // Current branch indicator
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0, 0, 0)
                [SNew(STextBlock)
                    .Text(InItem->bIsCurrent ? FText::FromString(TEXT("*")) : FText::GetEmpty())
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))]

            // Branch name
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2, 0, 0, 0)
                [SNew(STextBlock)
                    .Text(FText::FromString(InItem->Name))
                    .Font(InItem->bIsCurrent ?
                        FCoreStyle::GetDefaultFontStyle("Bold", 10) :
                        FCoreStyle::GetDefaultFontStyle("Regular", 10))]

            // Status suffix
            + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2, 0, 5, 0)
                [SNew(STextBlock)
                    .Text(FText::FromString(StatusSuffix))
                    .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
                    .ColorAndOpacity(StatusColor)]
        ];
}

FReply SCustomGitBranchPanel::OnCreateBranchClicked()
{
    FString NewName = NewBranchNameInput->GetText().ToString();
    if (NewName.IsEmpty())
    {
        return FReply::Handled();
    }

    if (OnCreateBranchDelegate.IsBound())
    {
        OnCreateBranchDelegate.Execute(NewName);
        NewBranchNameInput->SetText(FText::GetEmpty());
    }

    return FReply::Handled();
}

TSharedPtr<SWidget> SCustomGitBranchPanel::OnOpenBranchContextMenu()
{
    if (!BranchListView.IsValid())
    {
        return nullptr;
    }

    TArray<TSharedPtr<FGitBranchInfo>> SelectedBranches = BranchListView->GetSelectedItems();
    if (SelectedBranches.Num() == 0)
    {
        return nullptr;
    }

    FString BranchName = SelectedBranches[0]->Name;

    // Check if branch has upstream tracking
    TArray<FString> UpstreamResults, UpstreamErrors;
    bool bHasUpstream = false;
    FCustomGitOperations::RunGitCommand(
        TEXT("config"),
        {FString::Printf(TEXT("branch.%s.remote"), *BranchName)},
        {},
        UpstreamResults,
        UpstreamErrors);
    bHasUpstream = (UpstreamResults.Num() > 0 && !UpstreamResults[0].IsEmpty());

    FMenuBuilder MenuBuilder(true, nullptr);

    // Switch to branch
    MenuBuilder.AddMenuEntry(
        LOCTEXT("SwitchBranch", "Switch to Branch"),
        LOCTEXT("SwitchBranchTooltip", "Switch to the selected branch"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
            {
                if (SelectedBranches.Num() > 0 && OnSwitchBranchDelegate.IsBound())
                {
                    OnSwitchBranchDelegate.Execute(SelectedBranches[0]->Name);
                }
            })));

    MenuBuilder.AddSeparator();

    // Show "Push branch to origin" only if no upstream exists (local-only branch)
    if (!bHasUpstream)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("PushBranchToOrigin", "Push Branch to Origin"),
            LOCTEXT("PushBranchToOriginTooltip", "Push this local branch to origin for the first time"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this, BranchName]()
                {
                    if (OnPushBranchDelegate.IsBound())
                    {
                        OnPushBranchDelegate.Execute(BranchName);
                    }
                })));
    }

    // Always show Push option
    MenuBuilder.AddMenuEntry(
        LOCTEXT("PushBranch", "Push"),
        LOCTEXT("PushBranchTooltip", "Push commits on this branch to origin"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, BranchName]()
            {
                if (OnPushBranchDelegate.IsBound())
                {
                    OnPushBranchDelegate.Execute(BranchName);
                }
            })));

    MenuBuilder.AddSeparator();

    // Delete branch
    MenuBuilder.AddMenuEntry(
        LOCTEXT("DeleteBranch", "Delete Branch"),
        LOCTEXT("DeleteBranchTooltip", "Delete the selected branch"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
            {
                if (SelectedBranches.Num() > 0)
                {
                    FText Message = FText::Format(
                        LOCTEXT("DeleteBranchConfirm", "Are you sure you want to delete branch '{0}'?"),
                        FText::FromString(SelectedBranches[0]->Name));
                    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
                    if (Result == EAppReturnType::Yes && OnDeleteBranchDelegate.IsBound())
                    {
                        OnDeleteBranchDelegate.Execute(SelectedBranches[0]->Name);
                    }
                }
            })));

    MenuBuilder.AddSeparator();

    // Reset hard to origin
    MenuBuilder.AddMenuEntry(
        LOCTEXT("ResetToOrigin", "Reset Hard to Origin"),
        LOCTEXT("ResetToOriginTooltip", "Reset the selected branch to match origin (DESTRUCTIVE)"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateLambda([this, SelectedBranches]()
            {
                if (SelectedBranches.Num() > 0)
                {
                    FText Message = FText::Format(
                        LOCTEXT("ResetHardConfirm",
                            "WARNING: This will PERMANENTLY discard all local changes on branch '{0}' and reset to origin.\n\nAre you sure?"),
                        FText::FromString(SelectedBranches[0]->Name));
                    EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Message);
                    if (Result == EAppReturnType::Yes && OnResetBranchDelegate.IsBound())
                    {
                        OnResetBranchDelegate.Execute(SelectedBranches[0]->Name);
                    }
                }
            })));

    return MenuBuilder.MakeWidget();
}

void SCustomGitBranchPanel::RefreshBranchList()
{
    if (BranchListView.IsValid())
    {
        BranchListView->RequestListRefresh();
    }
}

void SCustomGitBranchPanel::SetCurrentBranchName(const FString& InBranchName)
{
    CurrentBranchName = InBranchName;
}

#undef LOCTEXT_NAMESPACE
