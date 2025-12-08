#include "CustomGitSourceControlState.h"

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

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::FindHistoryRevision(const FString& Revision) const
{
    return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FCustomGitSourceControlState::GetCurrentRevision() const
{
    return nullptr;
}

FName FCustomGitSourceControlState::GetIconName() const
{
    return FName();
}

FName FCustomGitSourceControlState::GetSmallIconName() const
{
    return FName();
}

FText FCustomGitSourceControlState::GetDisplayName() const
{
    return FText::FromString(Filename);
}

FText FCustomGitSourceControlState::GetDisplayTooltip() const
{
    return FText::FromString(Filename);
}

const FString& FCustomGitSourceControlState::GetFilename() const
{
    return Filename;
}

const FDateTime& FCustomGitSourceControlState::GetTimeStamp() const
{
    return TimeStamp;
}

bool FCustomGitSourceControlState::CanCheckout() const
{
    return false;
}

bool FCustomGitSourceControlState::IsCheckedOut() const
{
    return bIsCheckedOut;
}

bool FCustomGitSourceControlState::IsCheckedOutOther(FString* Who) const
{
    return false;
}

bool FCustomGitSourceControlState::IsCurrent() const
{
    return true;
}

bool FCustomGitSourceControlState::IsSourceControlled() const
{
    return bIsSourceControlled;
}

bool FCustomGitSourceControlState::IsAdded() const
{
    return false;
}

bool FCustomGitSourceControlState::IsDeleted() const
{
    return false;
}

bool FCustomGitSourceControlState::IsIgnored() const
{
    return false;
}

bool FCustomGitSourceControlState::CanEdit() const
{
    return true;
}

bool FCustomGitSourceControlState::CanDelete() const
{
    return true;
}

bool FCustomGitSourceControlState::IsUnknown() const
{
    return !bIsSourceControlled;
}

bool FCustomGitSourceControlState::IsModified() const
{
    return bIsModified;
}

bool FCustomGitSourceControlState::CanAdd() const
{
    return !bIsSourceControlled;
}

bool FCustomGitSourceControlState::IsConflicted() const
{
    return false;
}

bool FCustomGitSourceControlState::CanRevert() const
{
    return bIsModified;
}

bool FCustomGitSourceControlState::CanCheckIn() const
{
    return bIsModified;
}

// New implementations

FSlateIcon FCustomGitSourceControlState::GetIcon() const
{
    return FSlateIcon();
}

bool FCustomGitSourceControlState::IsCheckedOutInOtherBranch(const FString& CurrentBranch) const
{
    return false;
}

bool FCustomGitSourceControlState::IsModifiedInOtherBranch(const FString& CurrentBranch) const
{
    return false;
}

bool FCustomGitSourceControlState::IsCheckedOutOrModifiedInOtherBranch(const FString& CurrentBranch) const
{
    return false;
}

TArray<FString> FCustomGitSourceControlState::GetCheckedOutBranches() const
{
    return TArray<FString>();
}

FString FCustomGitSourceControlState::GetOtherUserBranchCheckedOuts() const
{
    return FString();
}

bool FCustomGitSourceControlState::GetOtherBranchHeadModification(FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut) const
{
    return false;
}


#undef LOCTEXT_NAMESPACE
