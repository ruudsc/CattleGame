#include "CustomGitSourceControlProvider.h"
#include "ISourceControlModule.h"
#include "CustomGitModule.h"
#include "CustomGitSourceControlCommand.h"
#include "CustomGitSourceControlState.h" // Added include
#include "HAL/RunnableThread.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "CustomGitOperations.h"

#define LOCTEXT_NAMESPACE "CustomGitSourceControl"

void FCustomGitSourceControlProvider::Init(bool bForceConnection)
{
    if (bForceConnection)
    {
        if (FCustomGitOperations::CheckGitAvailability())
        {
            // Success
        }
    }
}

void FCustomGitSourceControlProvider::Close()
{
}

FText FCustomGitSourceControlProvider::GetStatusText() const
{
    return LOCTEXT("CustomGitStatus", "Enabled");
}

TMap<ISourceControlProvider::EStatus, FString> FCustomGitSourceControlProvider::GetStatus() const
{
    TMap<EStatus, FString> Status;
    Status.Add(EStatus::Enabled, TEXT("True"));
    Status.Add(EStatus::Connected, TEXT("True"));
    return Status;
}

bool FCustomGitSourceControlProvider::IsEnabled() const
{
    return true;
}

bool FCustomGitSourceControlProvider::IsAvailable() const
{
    return true;
}

const FName &FCustomGitSourceControlProvider::GetName(void) const
{
    static FName CustomGitName("CustomGit");
    return CustomGitName;
}

bool FCustomGitSourceControlProvider::QueryStateBranchConfig(const FString &ConfigSrc, const FString &ConfigDest)
{
    return false;
}

void FCustomGitSourceControlProvider::RegisterStateBranches(const TArray<FString> &BranchNames, const FString &ContentRoot)
{
}

int32 FCustomGitSourceControlProvider::GetStateBranchIndex(const FString &BranchName) const
{
    return INDEX_NONE;
}

bool FCustomGitSourceControlProvider::GetStateBranchAtIndex(int32 BranchIndex, FString &OutBranchName) const
{
    return false;
}

ECommandResult::Type FCustomGitSourceControlProvider::GetState(const TArray<FString> &InFiles, TArray<FSourceControlStateRef> &OutState, EStateCacheUsage::Type InStateCacheUsage)
{
    if (InStateCacheUsage == EStateCacheUsage::ForceUpdate)
    {
        // Actually run update status?
        // For simple impl, just read cache or return current known state.
        // We should run the command 'UpdateStatus' but that is async usually.
        // If synchronous update requested, we might need to block.
        // For now, return what we have in cache.
    }

    for (const FString &File : InFiles)
    {
        if (StateCache.Contains(File))
        {
            OutState.Add(StateCache[File]);
        }
        else
        {
            OutState.Add(MakeShareable(new FCustomGitSourceControlState(File)));
        }
    }
    return ECommandResult::Succeeded; // Assuming we provided what we have
}

ECommandResult::Type FCustomGitSourceControlProvider::GetState(const TArray<UPackage *> &InPackages, TArray<FSourceControlStateRef> &OutState, EStateCacheUsage::Type InStateCacheUsage)
{
    TArray<FString> Files;
    for (UPackage *Package : InPackages)
    {
        Files.Add(Package->GetLoadedPath().GetLocalFullPath());
    }
    return GetState(Files, OutState, InStateCacheUsage);
}

FSourceControlStatePtr FCustomGitSourceControlProvider::GetState(const UPackage *InPackage, EStateCacheUsage::Type InStateCacheUsage)
{
    TArray<FSourceControlStateRef> OutState;
    TArray<UPackage *> Packages;
    Packages.Add(const_cast<UPackage *>(InPackage));
    GetState(Packages, OutState, InStateCacheUsage);
    if (OutState.Num() > 0)
    {
        return OutState[0];
    }
    return nullptr;
}

FSourceControlChangelistStatePtr FCustomGitSourceControlProvider::GetState(const FSourceControlChangelistRef &InChangelist, EStateCacheUsage::Type InStateCacheUsage)
{
    return nullptr;
}

ECommandResult::Type FCustomGitSourceControlProvider::GetState(const TArray<FSourceControlChangelistRef> &InChangelists, TArray<FSourceControlChangelistStateRef> &OutState, EStateCacheUsage::Type InStateCacheUsage)
{
    return ECommandResult::Succeeded;
}

TArray<FSourceControlStateRef> FCustomGitSourceControlProvider::GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef &)> Predicate) const
{
    TArray<FSourceControlStateRef> Result;
    for (const auto &Pair : StateCache)
    {
        if (Predicate(Pair.Value))
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

FDelegateHandle FCustomGitSourceControlProvider::RegisterSourceControlStateChanged_Handle(const FSourceControlStateChanged::FDelegate &SourceControlStateChanged)
{
    return OnSourceControlStateChanged.Add(SourceControlStateChanged);
}

void FCustomGitSourceControlProvider::UnregisterSourceControlStateChanged_Handle(FDelegateHandle Handle)
{
    OnSourceControlStateChanged.Remove(Handle);
}

ECommandResult::Type FCustomGitSourceControlProvider::Execute(const FSourceControlOperationRef &InOperation, FSourceControlChangelistPtr InChangelist, const TArray<FString> &InFiles, EConcurrency::Type InConcurrency, const FSourceControlOperationComplete &InOperationCompleteDelegate)
{
    // Create the command worker
    FCustomGitSourceControlCommand *Command = new FCustomGitSourceControlCommand(InOperation, InFiles, InOperationCompleteDelegate);

    // Create the thread to run the command
    if (InConcurrency == EConcurrency::Synchronous)
    {
        Command->Init();
        Command->Run();
        Command->Exit();
        return ECommandResult::Succeeded;
    }
    else
    {
        FRunnableThread::Create(Command, TEXT("CustomGitSourceControlCommand"));
        return ECommandResult::Succeeded;
    }
}

bool FCustomGitSourceControlProvider::CanExecuteOperation(const FSourceControlOperationRef &InOperation) const
{
    return true;
}

bool FCustomGitSourceControlProvider::CanCancelOperation(const FSourceControlOperationRef &InOperation) const
{
    return false;
}

void FCustomGitSourceControlProvider::CancelOperation(const FSourceControlOperationRef &InOperation)
{
}

bool FCustomGitSourceControlProvider::UsesLocalReadOnlyState() const
{
    // Enable Perforce-style read-only workflow
    // Files locked by others will be read-only on disk
    return true;
}

bool FCustomGitSourceControlProvider::UsesChangelists() const
{
    return false;
}

bool FCustomGitSourceControlProvider::UsesUncontrolledChangelists() const
{
    return false;
}

bool FCustomGitSourceControlProvider::UsesCheckout() const
{
    return true;
}

bool FCustomGitSourceControlProvider::UsesFileRevisions() const
{
    return false;
}

bool FCustomGitSourceControlProvider::UsesSnapshots() const
{
    return false;
}

bool FCustomGitSourceControlProvider::AllowsDiffAgainstDepot() const
{
    return false;
}

TOptional<bool> FCustomGitSourceControlProvider::IsAtLatestRevision() const
{
    return TOptional<bool>();
}

TOptional<int> FCustomGitSourceControlProvider::GetNumLocalChanges() const
{
    return TOptional<int>();
}

void FCustomGitSourceControlProvider::Tick()
{
}

TArray<TSharedRef<class ISourceControlLabel>> FCustomGitSourceControlProvider::GetLabels(const FString &InMatchingSpec) const
{
    return TArray<TSharedRef<class ISourceControlLabel>>();
}

TArray<FSourceControlChangelistRef> FCustomGitSourceControlProvider::GetChangelists(EStateCacheUsage::Type InStateCacheUsage)
{
    return TArray<FSourceControlChangelistRef>();
}

TSharedRef<class SWidget> FCustomGitSourceControlProvider::MakeSettingsWidget() const
{
    return SNew(STextBlock).Text(LOCTEXT("CustomGitSettings", "Custom Git Plugin Settings (None)"));
}

TSharedPtr<ISourceControlState, ESPMode::ThreadSafe> FCustomGitSourceControlProvider::GetState(const FString &Filename, EStateCacheUsage::Type CacheUsage)
{
    if (StateCache.Contains(Filename))
    {
        return StateCache[Filename];
    }
    return nullptr;
}

void FCustomGitSourceControlProvider::UpdateStateCache(const TMap<FString, FString> &Statuses)
{
    // Get current user name once for all comparisons
    FString CurrentUserName = FCustomGitOperations::GetCurrentUserName();

    for (const auto &Pair : Statuses)
    {
        const FString &Filename = Pair.Key;
        const FString &Status = Pair.Value;

        if (!StateCache.Contains(Filename))
        {
            StateCache.Add(Filename, MakeShareable(new FCustomGitSourceControlState(Filename)));
        }

        TSharedRef<FCustomGitSourceControlState, ESPMode::ThreadSafe> State = StateCache[Filename];

        // Parse git status codes
        // Status format: "XY" where X=index state, Y=worktree state
        // Or contains "|LOCKED:username"

        FString GitStatus = Status;
        FString LockOwner;

        // Check for lock info appended to status
        int32 LockIndex = Status.Find(TEXT("|LOCKED:"));
        if (LockIndex != INDEX_NONE)
        {
            GitStatus = Status.Left(LockIndex);
            LockOwner = Status.RightChop(LockIndex + 8); // Skip "|LOCKED:"
        }
        else if (Status.StartsWith(TEXT("LOCKED:")))
        {
            // File is only known because it's locked (no git status change)
            GitStatus.Empty();
            LockOwner = Status.RightChop(7); // Skip "LOCKED:"
        }

        // Parse git status codes using the new helper
        if (!GitStatus.IsEmpty())
        {
            State->SetWorkingCopyStateFromGitStatus(GitStatus);
        }
        else
        {
            // Default to unchanged if no git status
            State->WorkingCopyState = EGitWorkingCopyState::Unchanged;
        }

        // Process lock information
        if (!LockOwner.IsEmpty())
        {
            // File is tracked by LFS
            State->bIsLFSTracked = true;

            // Check if locked by current user or another user
            if (LockOwner.Equals(CurrentUserName, ESearchCase::IgnoreCase))
            {
                // Locked by us - checked out
                State->SetLockState(EGitLFSLockState::LockedByMe);
            }
            else
            {
                // Locked by someone else
                State->SetLockState(EGitLFSLockState::LockedByOther, LockOwner);
            }
        }
        else
        {
            // Not locked
            State->SetLockState(EGitLFSLockState::NotLocked);
        }

        // Check if file is LFS trackable based on extension
        if (!State->bIsLFSTracked)
        {
            State->bIsLFSTracked = FCustomGitOperations::IsBinaryAsset(Filename);
        }

        // Update timestamp
        State->TimeStamp = FDateTime::Now();
    }

    // Notify listeners that state changed
    OnSourceControlStateChanged.Broadcast();
}

#undef LOCTEXT_NAMESPACE
