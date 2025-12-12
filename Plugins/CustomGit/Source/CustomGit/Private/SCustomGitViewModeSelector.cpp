#include "SCustomGitViewModeSelector.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "SCustomGitViewModeSelector"

void SCustomGitViewModeSelector::Construct(const FArguments& InArgs)
{
    CurrentViewMode = InArgs._InitialViewMode;
    OnViewModeChangedDelegate = InArgs._OnViewModeChanged;

    // Initialize view mode list
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Local Changes"), EGitViewMode::LocalChanges}));
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Staged Changes"), EGitViewMode::StagedChanges}));
    ViewModeList.Add(MakeShareable(new FViewModeItem{TEXT("Locked Files"), EGitViewMode::LockedFiles}));

    ChildSlot
        [SNew(SVerticalBox)

            // Header
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2)
                [SNew(STextBlock)
                    .Text(LOCTEXT("CurrentChangesHeader", "Current Changes"))
                    .Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))]

            // View mode list
            + SVerticalBox::Slot()
                .AutoHeight()
                [SAssignNew(ViewModeListView, SListView<TSharedPtr<FViewModeItem>>)
                    .ListItemsSource(&ViewModeList)
                    .OnGenerateRow(this, &SCustomGitViewModeSelector::OnGenerateViewModeRow)
                    .OnSelectionChanged(this, &SCustomGitViewModeSelector::OnViewModeSelectionChanged)
                    .SelectionMode(ESelectionMode::Single)]
        ];

    // Set initial selection
    if (ViewModeListView.IsValid() && ViewModeList.Num() > 0)
    {
        ViewModeListView->SetSelection(ViewModeList[0], ESelectInfo::Direct);
    }
}

TSharedRef<ITableRow> SCustomGitViewModeSelector::OnGenerateViewModeRow(
    TSharedPtr<FViewModeItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    bool bIsSelected = (CurrentViewMode == InItem->ViewMode);

    return SNew(STableRow<TSharedPtr<FViewModeItem>>, OwnerTable)
        [SNew(SBorder)
            .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
            .Padding(FMargin(5, 2))
            [SNew(STextBlock)
                .Text(FText::FromString(InItem->Name))
                .Font(bIsSelected ?
                    FCoreStyle::GetDefaultFontStyle("Bold", 10) :
                    FCoreStyle::GetDefaultFontStyle("Regular", 10))]
        ];
}

void SCustomGitViewModeSelector::OnViewModeSelectionChanged(
    TSharedPtr<FViewModeItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (!SelectedItem.IsValid() || SelectInfo == ESelectInfo::Direct)
    {
        return;
    }

    SetViewMode(SelectedItem->ViewMode, true);
}

void SCustomGitViewModeSelector::SetViewMode(EGitViewMode NewMode, bool bNotify)
{
    if (CurrentViewMode == NewMode)
    {
        return;
    }

    CurrentViewMode = NewMode;

    // Update list view selection
    if (ViewModeListView.IsValid() && ViewModeList.Num() > 0)
    {
        for (int32 i = 0; i < ViewModeList.Num(); ++i)
        {
            if (ViewModeList[i]->ViewMode == NewMode)
            {
                ViewModeListView->SetSelection(ViewModeList[i], ESelectInfo::Direct);
                ViewModeListView->RequestListRefresh();
                break;
            }
        }
    }

    // Fire delegate if requested
    if (bNotify && OnViewModeChangedDelegate.IsBound())
    {
        OnViewModeChangedDelegate.Execute(NewMode);
    }
}

#undef LOCTEXT_NAMESPACE
