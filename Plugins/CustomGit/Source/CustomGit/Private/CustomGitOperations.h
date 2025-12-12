#pragma once

#include "CoreMinimal.h"

// Struct to hold branch information including remote tracking status
struct FGitBranchInfo
{
    FString Name;
    bool bIsCurrent = false;
    bool bIsLocal = false;        // True if branch has never been pushed (no upstream)
    bool bIsUpstreamGone = false; // True if upstream branch was deleted
};

class FCustomGitOperations
{
public:
    /**
     * Run a git command.
     * @param Command - The git command (e.g. "commit", "status")
     * @param Parameters - List of parameters
     * @param Files - List of files to operate on
     * @param OutResults - Output lines from stdout
     * @param OutErrors - Output lines from stderr
     * @return true if command succeeded
     */
    static bool RunGitCommand(const FString &Command, const TArray<FString> &Parameters, const TArray<FString> &Files, TArray<FString> &OutResults, TArray<FString> &OutErrors);

    /**
     * Run a git lfs command.
     */
    static bool RunGitLFSCommand(const FString &Command, const TArray<FString> &Parameters, const TArray<FString> &Files, TArray<FString> &OutResults, TArray<FString> &OutErrors);

    /**
     * Check if git is available
     */
    static bool CheckGitAvailability();

    /**
     * Run git status and parse results
     */
    static void UpdateStatus(const TArray<FString> &Files, TMap<FString, FString> &OutStatuses); // Filename -> "M", "A", "??", etc.

    /**
     * Lock a file using LFS
     */
    static bool LockFile(const FString &File, FString &OutError);

    /**
     * Unlock a file using LFS
     */
    static bool UnlockFile(const FString &File, FString &OutError);

    /**
     * Get all LFS locks
     * Map: Filename -> Owner
     */
    static void GetAllLocks(TMap<FString, FString> &OutLocks);

    /**
     * Get all LFS locks with ownership info using --verify
     * @param OutOurLocks - Set of file paths that WE own locks for
     * @param OutOtherLocks - Map of file paths to owner names for locks owned by others
     */
    static void GetLocksWithOwnership(TSet<FString> &OutOurLocks, TMap<FString, FString> &OutOtherLocks);

    /**
     * Revert a file (unlock and checkout)
     */
    static bool RevertFile(const FString &File, FString &OutError);

    /**
     * Commit specific files
     */
    static bool Commit(const FString &Message, const TArray<FString> &Files, FString &OutError);

    /**
     * Push to remote
     */
    static bool Push(FString &OutError);

    /**
     * Pull from remote
     */
    static bool Pull(FString &OutError);

    /**
     * Get the current git user name and email
     */
    static void GetUserInfo(FString &OutUserName, FString &OutEmail);

    /**
     * Get list of branches with tracking status
     * @param OutBranches - Array of branch info including tracking status
     * @param OutCurrentBranch - Returns the name of the currently checked out branch
     */
    static void GetBranches(TArray<FGitBranchInfo> &OutBranches, FString &OutCurrentBranch);

    /**
     * Get commit history (git log)
     * @param MaxCount - Maximum number of commits to return
     * @param OutCommits - Array of commit strings (hash - message)
     */
    static void GetCommitHistory(int32 MaxCount, TArray<FString> &OutCommits);

    /**
     * Create a new branch
     */
    static bool CreateBranch(const FString &BranchName, FString &OutError);

    /**
     * Switch to a branch
     */
    static bool SwitchBranch(const FString &BranchName, FString &OutError);

    /**
     * Merge a branch
     */
    static bool Merge(const FString &BranchName, FString &OutError);

    /**
     * Get the last modified timestamp of a file
     */
    static FDateTime GetFileLastModified(const FString &File);

    /**
     * Get command history
     */
    static const TArray<FString> &GetCommandHistory();

    /**
     * Get the repository root path
     */
    static FString GetRepositoryRoot();

    /**
     * Check if a file is tracked by Git (not untracked/new)
     */
    static bool IsFileTrackedByGit(const FString &File);

    /**
     * Check if a file is tracked by Git LFS
     */
    static bool IsLFSTrackedFile(const FString &File);

    /**
     * Check if a file is a binary asset type (uasset, umap, etc.)
     */
    static bool IsBinaryAsset(const FString &File);

    /**
     * Lock a file if it's tracked by LFS, with optional warning callback
     * Returns true if lock was acquired or file is not LFS tracked
     */
    static bool LockFileIfLFS(const FString &File, FString &OutError, bool bShowWarning = true);

    /**
     * Force lock a file using LFS (steals lock from other user)
     * Use with caution - only when user explicitly requests force checkout
     */
    static bool ForceLockFile(const FString &File, FString &OutError);

    /**
     * Set a file as read-only on the filesystem
     */
    static bool SetFileReadOnly(const FString &File, bool bReadOnly);

    /**
     * Get the current git user name (from git config user.name)
     */
    static FString GetCurrentUserName();

    /**
     * Check if the current user owns the lock for a file
     * @param File - The file path to check
     * @param OutOwner - If locked, the owner's name
     * @return true if the current user owns the lock, false otherwise
     */
    static bool IsLockedByCurrentUser(const FString &File, FString *OutOwner = nullptr);

    /**
     * Invalidate the lock cache (call after lock/unlock operations)
     */
    static void InvalidateLockCache();

private:
    static TArray<FString> CommandHistory;
    static FString RepositoryRoot;
    static FString CachedUserName;

    // Lock caching for performance (Issue #3 from performance review)
    static TMap<FString, FString> CachedLocks;
    static TSet<FString> CachedOurLocks;
    static TMap<FString, FString> CachedOtherLocks;
    static double LastLocksCacheTime;
    static double LastOwnershipCacheTime;
    static constexpr double LOCKS_CACHE_DURATION = 2.0; // Cache locks for 2 seconds

    static void AddToHistory(const FString &Command);
};
