#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Result of the lock file dialog
 */
enum class ELockFileDialogResult : uint8
{
    Lock,            // User chose to lock the file
    SaveWithoutLock, // User chose to save without locking
    Cancel           // User cancelled the save operation
};

/**
 * Custom dialog for prompting the user about locking a binary file before saving
 */
class SLockFileDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLockFileDialog)
        : _Filename(), _ErrorMessage(), _bIsAlreadyLocked(false), _LockedByUser()
    {
    }
    SLATE_ARGUMENT(FString, Filename)
    SLATE_ARGUMENT(FString, ErrorMessage)
    SLATE_ARGUMENT(bool, bIsAlreadyLocked)
    SLATE_ARGUMENT(FString, LockedByUser)
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs);

    /**
     * Shows the dialog and returns the user's choice
     * @param Filename The name of the file being saved
     * @param ErrorMessage Optional error message from lock attempt
     * @param bIsAlreadyLocked True if the file is locked by another user
     * @param LockedByUser The username who has the file locked (if applicable)
     * @return The user's choice
     */
    static ELockFileDialogResult ShowDialog(
        const FString &Filename,
        const FString &ErrorMessage = FString(),
        bool bIsAlreadyLocked = false,
        const FString &LockedByUser = FString());

private:
    FReply OnLockClicked();
    FReply OnSaveWithoutLockClicked();
    FReply OnCancelClicked();

    void CloseDialog();

    ELockFileDialogResult Result;
    TSharedPtr<SWindow> ParentWindow;
};
