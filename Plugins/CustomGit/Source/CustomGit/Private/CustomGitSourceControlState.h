#pragma once

#include "CoreMinimal.h"
#include "ISourceControlState.h"

/**
 * Enum representing the LFS lock state of a file
 */
enum class EGitLFSLockState : uint8
{
    NotLocked,    // File is not locked
    LockedByMe,   // File is locked by the current user (checked out)
    LockedByOther // File is locked by another user
};

/**
 * Enum representing the working copy state of a file
 */
enum class EGitWorkingCopyState : uint8
{
    Unknown,   // Not known to git
    Unchanged, // No local modifications
    Added,     // Newly added (staged)
    Deleted,   // Deleted
    Modified,  // Modified (staged or unstaged)
    Renamed,   // Renamed
    Copied,    // Copied
    Untracked, // Not tracked by git
    Ignored,   // Ignored by .gitignore
    Conflicted // Merge conflict
};

/**
 * Custom Git source control state that simulates Perforce-style exclusive checkout using Git LFS locks
 */
class FCustomGitSourceControlState : public ISourceControlState
{
public:
    FCustomGitSourceControlState(const FString &InFilename)
        : Filename(InFilename), LockState(EGitLFSLockState::NotLocked), WorkingCopyState(EGitWorkingCopyState::Unknown), bIsLFSTracked(false)
    {
    }

    // ISourceControlState interface
    virtual int32 GetHistorySize() const override;
    virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetHistoryItem(int32 HistoryIndex) const override;
    virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(int32 RevisionNumber) const override;
    virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(const FString &Revision) const override;
    virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetCurrentRevision() const override;

    virtual FSlateIcon GetIcon() const override;
    virtual bool IsCheckedOutInOtherBranch(const FString &CurrentBranch = FString()) const override;
    virtual bool IsModifiedInOtherBranch(const FString &CurrentBranch = FString()) const override;
    virtual bool IsCheckedOutOrModifiedInOtherBranch(const FString &CurrentBranch = FString()) const override;
    virtual TArray<FString> GetCheckedOutBranches() const override;
    virtual FString GetOtherUserBranchCheckedOuts() const override;
    virtual bool GetOtherBranchHeadModification(FString &HeadBranchOut, FString &ActionOut, int32 &HeadChangeListOut) const override;

    virtual FName GetIconName() const override;
    virtual FName GetSmallIconName() const override;
    virtual FText GetDisplayName() const override;
    virtual FText GetDisplayTooltip() const override;
    virtual const FString &GetFilename() const override;
    virtual const FDateTime &GetTimeStamp() const override;

    /** Can we check out this file? (i.e., can we acquire the LFS lock?) */
    virtual bool CanCheckout() const override;

    /** Is this file checked out by us? (i.e., do we hold the LFS lock?) */
    virtual bool IsCheckedOut() const override;

    /** Is this file checked out by someone else? (i.e., does someone else hold the LFS lock?) */
    virtual bool IsCheckedOutOther(FString *Who = nullptr) const override;

    virtual bool IsCurrent() const override;
    virtual bool IsSourceControlled() const override;
    virtual bool IsAdded() const override;
    virtual bool IsDeleted() const override;
    virtual bool IsIgnored() const override;

    /** Can we edit this file? Only if not locked by someone else */
    virtual bool CanEdit() const override;

    virtual bool CanDelete() const override;
    virtual bool IsUnknown() const override;
    virtual bool IsModified() const override;
    virtual bool CanAdd() const override;
    virtual bool IsConflicted() const override;
    virtual bool CanRevert() const override;
    virtual bool CanCheckIn() const override;

public:
    /** The filename this state represents */
    FString Filename;

    /** The current LFS lock state */
    EGitLFSLockState LockState;

    /** The username who has the file locked (if LockedByOther) */
    FString LockedByUser;

    /** The working copy state of the file */
    EGitWorkingCopyState WorkingCopyState;

    /** Is this file tracked by Git LFS? */
    bool bIsLFSTracked;

    /** Timestamp of last status update */
    FDateTime TimeStamp;

    /** Helper to set state from git status output */
    void SetWorkingCopyStateFromGitStatus(const FString &StatusCode);

    /** Helper to set lock state */
    void SetLockState(EGitLFSLockState NewState, const FString &LockOwner = FString());
};
