#include "SLockFileDialog.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SLockFileDialog"

void SLockFileDialog::Construct(const FArguments &InArgs)
{
    Result = ELockFileDialogResult::Cancel;

    // Store whether file is locked by another user (used to hide Lock button)
    bool bLockedByOther = InArgs._bIsAlreadyLocked && !InArgs._LockedByUser.IsEmpty();

    // Build the message based on context
    FText MainMessage;
    FText DetailMessage;

    if (bLockedByOther)
    {
        // File is locked by another user
        MainMessage = FText::Format(
            LOCTEXT("LockedByOtherTitle", "File Locked by {0}"),
            FText::FromString(InArgs._LockedByUser));

        DetailMessage = LOCTEXT("LockedByOtherDetail",
                                "This binary file is currently locked by another team member.\n\n"
                                "You cannot lock this file until they release their lock.\n\n"
                                "• Save Without Locking: Save locally (you won't be able to push until they unlock)\n"
                                "• Cancel: Don't save the file");
    }
    else if (!InArgs._ErrorMessage.IsEmpty())
    {
        // Lock failed for some reason
        MainMessage = LOCTEXT("LockFailedTitle", "Could Not Lock File");

        DetailMessage = FText::Format(
            LOCTEXT("LockFailedDetail",
                    "Failed to lock the file: {0}\n\n"
                    "• Save and Lock: Retry locking the file\n"
                    "• Save Without Locking: Save locally, but may cause conflicts when pushing\n"
                    "• Cancel: Don't save the file"),
            FText::FromString(InArgs._ErrorMessage));
    }
    else
    {
        // Normal case - asking if user wants to lock
        MainMessage = LOCTEXT("LockFileTitle", "Lock Binary File?");

        DetailMessage = LOCTEXT("LockFileDetail",
                                "This is a binary file that cannot be merged if edited by multiple people.\n\n"
                                "Locking the file prevents others from editing it until you check it in.\n\n"
                                "• Save and Lock: Lock to get exclusive edit rights (recommended)\n"
                                "• Save Without Locking: Save without protection (may cause conflicts)\n"
                                "• Cancel: Don't save the file");
    }

    ChildSlot
        [SNew(SBorder)
             .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
             .Padding(16.0f)
                 [SNew(SVerticalBox)

                  // Icon and Title
                  + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                            [SNew(SHorizontalBox)

                             + SHorizontalBox::Slot()
                                   .AutoWidth()
                                   .VAlign(VAlign_Center)
                                   .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                                       [SNew(SImage)
                                            .Image(FAppStyle::GetBrush("Icons.WarningWithColor.Large"))]

                             + SHorizontalBox::Slot()
                                   .FillWidth(1.0f)
                                   .VAlign(VAlign_Center)
                                       [SNew(STextBlock)
                                            .Text(MainMessage)
                                            .Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))
                                            .ColorAndOpacity(FStyleColors::White)]]

                  // Filename
                  + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                            [SNew(SBorder)
                                 .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                                 .Padding(8.0f)
                                     [SNew(STextBlock)
                                          .Text(FText::FromString(InArgs._Filename))
                                          .Font(FAppStyle::Get().GetFontStyle("SmallFontBold"))
                                          .ColorAndOpacity(FStyleColors::AccentBlue)]]

                  // Detail message
                  + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 20.0f)
                            [SNew(STextBlock)
                                 .Text(DetailMessage)
                                 .AutoWrapText(true)
                                 .ColorAndOpacity(FStyleColors::Foreground)]

                  // Buttons
                  + SVerticalBox::Slot()
                        .AutoHeight()
                            [SNew(SHorizontalBox)

                             // Save and Lock button (Blue/Primary) - only show if NOT locked by another user
                             + SHorizontalBox::Slot()
                                   .AutoWidth()
                                   .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                                       [SNew(SButton)
                                            .Visibility(bLockedByOther ? EVisibility::Collapsed : EVisibility::Visible)
                                            .ButtonStyle(FAppStyle::Get(), "PrimaryButton")
                                            .OnClicked(this, &SLockFileDialog::OnLockClicked)
                                            .ContentPadding(FMargin(16.0f, 8.0f))
                                                [SNew(STextBlock)
                                                     .Text(LOCTEXT("LockFileButton", "Save and Lock"))
                                                     .ColorAndOpacity(FLinearColor::White)
                                                     .Font(FAppStyle::Get().GetFontStyle("SmallFontBold"))]]

                             // Save Without Locking button (Gray/Default)
                             + SHorizontalBox::Slot()
                                   .AutoWidth()
                                   .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                                       [SNew(SButton)
                                            .OnClicked(this, &SLockFileDialog::OnSaveWithoutLockClicked)
                                            .ContentPadding(FMargin(16.0f, 8.0f))
                                                [SNew(STextBlock)
                                                     .Text(LOCTEXT("SaveWithoutLockButton", "Save Without Locking"))]]

                             + SHorizontalBox::Slot()
                                   .FillWidth(1.0f)
                                       [SNew(SSpacer)]

                             // Cancel button (Red-ish)
                             + SHorizontalBox::Slot()
                                   .AutoWidth()
                                       [SNew(SButton)
                                            .OnClicked(this, &SLockFileDialog::OnCancelClicked)
                                            .ContentPadding(FMargin(16.0f, 8.0f))
                                            .ButtonColorAndOpacity(FLinearColor(0.6f, 0.1f, 0.1f, 1.0f))
                                                [SNew(STextBlock)
                                                     .Text(LOCTEXT("CancelButton", "Cancel"))
                                                     .ColorAndOpacity(FSlateColor(FLinearColor::White))]]]]];
}

ELockFileDialogResult SLockFileDialog::ShowDialog(
    const FString &Filename,
    const FString &ErrorMessage,
    bool bIsAlreadyLocked,
    const FString &LockedByUser)
{
    TSharedRef<SLockFileDialog> DialogContent = SNew(SLockFileDialog)
                                                    .Filename(Filename)
                                                    .ErrorMessage(ErrorMessage)
                                                    .bIsAlreadyLocked(bIsAlreadyLocked)
                                                    .LockedByUser(LockedByUser);

    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
                                           .Title(LOCTEXT("DialogWindowTitle", "Lock File"))
                                           .SizingRule(ESizingRule::Autosized)
                                           .AutoCenter(EAutoCenter::PreferredWorkArea)
                                           .SupportsMinimize(false)
                                           .SupportsMaximize(false)
                                               [DialogContent];

    DialogContent->ParentWindow = DialogWindow;

    // Show as modal
    GEditor->EditorAddModalWindow(DialogWindow);

    return DialogContent->Result;
}

FReply SLockFileDialog::OnLockClicked()
{
    Result = ELockFileDialogResult::Lock;
    CloseDialog();
    return FReply::Handled();
}

FReply SLockFileDialog::OnSaveWithoutLockClicked()
{
    Result = ELockFileDialogResult::SaveWithoutLock;
    CloseDialog();
    return FReply::Handled();
}

FReply SLockFileDialog::OnCancelClicked()
{
    Result = ELockFileDialogResult::Cancel;
    CloseDialog();
    return FReply::Handled();
}

void SLockFileDialog::CloseDialog()
{
    if (ParentWindow.IsValid())
    {
        ParentWindow->RequestDestroyWindow();
    }
}

#undef LOCTEXT_NAMESPACE
