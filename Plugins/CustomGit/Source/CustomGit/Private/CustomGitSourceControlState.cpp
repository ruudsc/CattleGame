#include "CustomGitSourceControlState.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "CustomGitSourceControlState"

int32 FCustomGitSourceControlState::GetHistorySize() const
{
    return 0;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::GetHistoryItem(int32 HistoryIndex) const
{
    return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::FindHistoryRevision(int32 RevisionNumber) const
{
    return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::FindHistoryRevision(const FString &Revision) const
{
    return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::GetCurrentRevision() const
{
    return nullptr;
}

FName FCustomGitSourceControlState::GetIconName() const
{
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        return FName("Perforce.CheckedOutByOtherUser");
    }
    else if (LockState == EGitLFSLockState::LockedByMe)
    {
        return FName("Perforce.CheckedOut");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Added)
    {
        return FName("Perforce.OpenForAdd");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Modified)
    {
        return FName("Perforce.CheckedOut");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Conflicted)
    {
        return FName("Perforce.NotAtHeadRevision");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Untracked)
    {
        return FName("Perforce.NotInDepot");
    }
    else if (IsSourceControlled())
    {
        return FName("Perforce.CheckedIn");
    }
    return FName();
}

FName FCustomGitSourceControlState::GetSmallIconName() const
{
    return GetIconName();
}

FText FCustomGitSourceControlState::GetDisplayName() const
{
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        return FText::Format(LOCTEXT("CheckedOutOther", "Checked Out by {0}"), FText::FromString(LockedByUser));
    }
    else if (LockState == EGitLFSLockState::LockedByMe)
    {
        return LOCTEXT("CheckedOut", "Checked Out");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Added)
    {
        return LOCTEXT("Added", "Added");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Deleted)
    {
        return LOCTEXT("Deleted", "Marked for Delete");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Modified)
    {
        return LOCTEXT("Modified", "Modified");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Conflicted)
    {
        return LOCTEXT("Conflicted", "Conflicted");
    }
    else if (WorkingCopyState == EGitWorkingCopyState::Untracked)
    {
        return LOCTEXT("NotControlled", "Not Under Source Control");
    }
    else if (IsSourceControlled())
    {
        return LOCTEXT("Controlled", "Source Controlled");
    }
    return LOCTEXT("Unknown", "Unknown");
}

FText FCustomGitSourceControlState::GetDisplayTooltip() const
{
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        return FText::Format(LOCTEXT("CheckedOutOtherTooltip", "This file is checked out by {0}. You cannot edit it until they check it in."), FText::FromString(LockedByUser));
    }
    else if (LockState == EGitLFSLockState::LockedByMe)
    {
        return LOCTEXT("CheckedOutTooltip", "You have this file checked out (locked). You can edit and submit changes.");
    }
    else if (bIsLFSTracked && LockState == EGitLFSLockState::NotLocked)
    {
        return LOCTEXT("NotCheckedOutTooltip", "This file is not checked out. Check it out before editing to prevent conflicts.");
    }
    return GetDisplayName();
}

const FString &FCustomGitSourceControlState::GetFilename() const
{
    return Filename;
}

const FDateTime &FCustomGitSourceControlState::GetTimeStamp() const
{
    return TimeStamp;
}

bool FCustomGitSourceControlState::CanCheckout() const
{
    // Can checkout if:
    // - File is source controlled
    // - Not already locked by us
    // - Not locked by someone else (unless force checkout)
    // For binary assets tracked by LFS, we require checkout before edit
    return IsSourceControlled() && LockState != EGitLFSLockState::LockedByMe;
}

bool FCustomGitSourceControlState::IsCheckedOut() const
{
    // Checked out = we hold the LFS lock
    return LockState == EGitLFSLockState::LockedByMe;
}

bool FCustomGitSourceControlState::IsCheckedOutOther(FString *Who) const
{
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        if (Who)
        {
            *Who = LockedByUser;
        }
        return true;
    }
    return false;
}

bool FCustomGitSourceControlState::IsCurrent() const
{
    // For now, assume we're current. Could check against remote HEAD.
    return true;
}

bool FCustomGitSourceControlState::IsSourceControlled() const
{
    return WorkingCopyState != EGitWorkingCopyState::Unknown &&
           WorkingCopyState != EGitWorkingCopyState::Untracked &&
           WorkingCopyState != EGitWorkingCopyState::Ignored;
}

bool FCustomGitSourceControlState::IsAdded() const
{
    return WorkingCopyState == EGitWorkingCopyState::Added;
}

bool FCustomGitSourceControlState::IsDeleted() const
{
    return WorkingCopyState == EGitWorkingCopyState::Deleted;
}

bool FCustomGitSourceControlState::IsIgnored() const
{
    return WorkingCopyState == EGitWorkingCopyState::Ignored;
}

bool FCustomGitSourceControlState::CanEdit() const
{
    // Can edit if:
    // - Not locked by someone else
    // - If it's an LFS tracked file, we should have the lock (checked out)
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        return false;
    }

    // For LFS tracked files, require checkout (lock) before editing
    if (bIsLFSTracked && LockState == EGitLFSLockState::NotLocked)
    {
        return false; // Must checkout first
    }

    return true;
}

bool FCustomGitSourceControlState::CanDelete() const
{
    // Can delete if we have it checked out or it's not locked by others
    return LockState != EGitLFSLockState::LockedByOther;
}

bool FCustomGitSourceControlState::IsUnknown() const
{
    return WorkingCopyState == EGitWorkingCopyState::Unknown;
}

bool FCustomGitSourceControlState::IsModified() const
{
    return WorkingCopyState == EGitWorkingCopyState::Modified ||
           WorkingCopyState == EGitWorkingCopyState::Added ||
           WorkingCopyState == EGitWorkingCopyState::Deleted ||
           WorkingCopyState == EGitWorkingCopyState::Renamed ||
           WorkingCopyState == EGitWorkingCopyState::Copied;
}

bool FCustomGitSourceControlState::CanAdd() const
{
    return WorkingCopyState == EGitWorkingCopyState::Untracked;
}

bool FCustomGitSourceControlState::IsConflicted() const
{
    return WorkingCopyState == EGitWorkingCopyState::Conflicted;
}

bool FCustomGitSourceControlState::CanRevert() const
{
    // Can revert if modified or if we have a lock
    return IsModified() || LockState == EGitLFSLockState::LockedByMe;
}

bool FCustomGitSourceControlState::CanCheckIn() const
{
    // Can check in if:
    // - We have the file checked out (locked) AND there are modifications
    // - OR the file is modified and not LFS tracked
    if (LockState == EGitLFSLockState::LockedByMe && IsModified())
    {
        return true;
    }

    // For non-LFS files, can check in if modified
    if (!bIsLFSTracked && IsModified())
    {
        return true;
    }

    return false;
}

// ISourceControlState extended interface implementations

FSlateIcon FCustomGitSourceControlState::GetIcon() const
{
    FName IconName = GetIconName();
    if (!IconName.IsNone())
    {
        return FSlateIcon(FAppStyle::GetAppStyleSetName(), IconName);
    }
    return FSlateIcon();
}

bool FCustomGitSourceControlState::IsCheckedOutInOtherBranch(const FString &CurrentBranch) const
{
    return false;
}

bool FCustomGitSourceControlState::IsModifiedInOtherBranch(const FString &CurrentBranch) const
{
    return false;
}

bool FCustomGitSourceControlState::IsCheckedOutOrModifiedInOtherBranch(const FString &CurrentBranch) const
{
    return false;
}

TArray<FString> FCustomGitSourceControlState::GetCheckedOutBranches() const
{
    return TArray<FString>();
}

FString FCustomGitSourceControlState::GetOtherUserBranchCheckedOuts() const
{
    if (LockState == EGitLFSLockState::LockedByOther)
    {
        return LockedByUser;
    }
    return FString();
}

bool FCustomGitSourceControlState::GetOtherBranchHeadModification(FString &HeadBranchOut, FString &ActionOut, int32 &HeadChangeListOut) const
{
    return false;
}

void FCustomGitSourceControlState::SetWorkingCopyStateFromGitStatus(const FString &StatusCode)
{
    // Parse git status --porcelain output
    // First character is staged status, second is unstaged status
    // Examples: "M " = staged modified, " M" = unstaged modified, "??" = untracked, "A " = staged add

    if (StatusCode.Len() < 2)
    {
        WorkingCopyState = EGitWorkingCopyState::Unknown;
        return;
    }

    TCHAR Staged = StatusCode[0];
    TCHAR Unstaged = StatusCode[1];

    // Check for untracked
    if (Staged == TEXT('?') && Unstaged == TEXT('?'))
    {
        WorkingCopyState = EGitWorkingCopyState::Untracked;
        return;
    }

    // Check for ignored
    if (Staged == TEXT('!') && Unstaged == TEXT('!'))
    {
        WorkingCopyState = EGitWorkingCopyState::Ignored;
        return;
    }

    // Check for conflicts
    if (Staged == TEXT('U') || Unstaged == TEXT('U') ||
        (Staged == TEXT('A') && Unstaged == TEXT('A')) ||
        (Staged == TEXT('D') && Unstaged == TEXT('D')))
    {
        WorkingCopyState = EGitWorkingCopyState::Conflicted;
        return;
    }

    // Check staged status first
    if (Staged == TEXT('A'))
    {
        WorkingCopyState = EGitWorkingCopyState::Added;
        return;
    }
    if (Staged == TEXT('D'))
    {
        WorkingCopyState = EGitWorkingCopyState::Deleted;
        return;
    }
    if (Staged == TEXT('R'))
    {
        WorkingCopyState = EGitWorkingCopyState::Renamed;
        return;
    }
    if (Staged == TEXT('C'))
    {
        WorkingCopyState = EGitWorkingCopyState::Copied;
        return;
    }
    if (Staged == TEXT('M'))
    {
        WorkingCopyState = EGitWorkingCopyState::Modified;
        return;
    }

    // Check unstaged status
    if (Unstaged == TEXT('M'))
    {
        WorkingCopyState = EGitWorkingCopyState::Modified;
        return;
    }
    if (Unstaged == TEXT('D'))
    {
        WorkingCopyState = EGitWorkingCopyState::Deleted;
        return;
    }

    // If we got here with no status flags, file is unchanged
    WorkingCopyState = EGitWorkingCopyState::Unchanged;
}

void FCustomGitSourceControlState::SetLockState(EGitLFSLockState NewState, const FString &LockOwner)
{
    LockState = NewState;
    if (NewState == EGitLFSLockState::LockedByOther)
    {
        LockedByUser = LockOwner;
    }
    else
    {
        LockedByUser.Empty();
    }
}

#undef LOCTEXT_NAMESPACE
