#pragma once

#include "CoreMinimal.h"

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
     * Get list of branches
     * @param OutCurrentBranch - Returns the name of the currently checked out branch
     */
    static void GetBranches(TArray<FString> &OutBranches, FString &OutCurrentBranch);

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

private:
    static TArray<FString> CommandHistory;
    static FString RepositoryRoot;
    static void AddToHistory(const FString &Command);
};
