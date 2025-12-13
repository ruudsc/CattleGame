#include "CustomGitSourceControlCommand.h"
#include "HAL/RunnableThread.h"
#include "CustomGitOperations.h"
#include "CustomGitModule.h"
#include "CustomGitSourceControlProvider.h"
#include "Async/Async.h"
#include "SourceControlOperations.h"

FCustomGitSourceControlCommand::FCustomGitSourceControlCommand(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation, const TArray<FString> &InFiles, const FSourceControlOperationComplete &InOperationCompleteDelegate)
    : Operation(InOperation), Files(InFiles), OperationCompleteDelegate(InOperationCompleteDelegate), Result(ECommandResult::Failed)
{
}

bool FCustomGitSourceControlCommand::Init()
{
    return true;
}

uint32 FCustomGitSourceControlCommand::Run()
{
    if (Operation->GetName() == "Connect")
    {
        if (FCustomGitOperations::CheckGitAvailability())
        {
            Result = ECommandResult::Succeeded;
        }
        else
        {
            Result = ECommandResult::Failed;
            ErrorMessages.Add(TEXT("Git not available"));
        }
    }
    else if (Operation->GetName() == "UpdateStatus")
    {
        FCustomGitOperations::UpdateStatus(Files, StatusResults);
        Result = ECommandResult::Succeeded;
    }
    else if (Operation->GetName() == "CheckOut")
    {
        // Perforce-style Check Out: Lock the file and make it writable
        // Issue #5 from performance review: Fetch all locks ONCE before the loop
        bool bAllSuccess = true;

        // Pre-fetch all lock ownership info (reduces N queries to 1)
        TSet<FString> OurLocks;
        TMap<FString, FString> OtherLocks;
        FCustomGitOperations::GetLocksWithOwnership(OurLocks, OtherLocks);
        FString CurrentUser = FCustomGitOperations::GetCurrentUserName();
        FString RepoRoot = FCustomGitOperations::GetRepositoryRoot();

        for (const FString &File : Files)
        {
            // Convert to relative path for lock comparison
            FString RelativePath = File;
            FPaths::MakePathRelativeTo(RelativePath, *(RepoRoot + TEXT("/")));
            RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));

            // Check in pre-fetched lock data
            if (OurLocks.Contains(RelativePath))
            {
                // Already locked by us, just make sure it's writable
                FCustomGitOperations::SetFileReadOnly(File, false);
                StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
                continue;
            }

            // Check if locked by someone else
            if (const FString *OtherOwner = OtherLocks.Find(RelativePath))
            {
                // File is locked by someone else
                ErrorMessages.Add(FString::Printf(TEXT("Cannot check out %s - locked by %s"), *FPaths::GetCleanFilename(File), **OtherOwner));
                bAllSuccess = false;
                continue;
            }

            // Not locked - try to lock the file
            FString Error;
            if (!FCustomGitOperations::LockFile(File, Error))
            {
                ErrorMessages.Add(FString::Printf(TEXT("Failed to lock %s: %s"), *FPaths::GetCleanFilename(File), *Error));
                bAllSuccess = false;
            }
            else
            {
                // Successfully locked - make the file writable
                FCustomGitOperations::SetFileReadOnly(File, false);

                // Update status for this file
                StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
            }
        }
        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
    }
    else if (Operation->GetName() == "ForceCheckOut")
    {
        // Force Check Out: Steal the lock from another user
        bool bAllSuccess = true;
        for (const FString &File : Files)
        {
            FString Error;
            if (!FCustomGitOperations::ForceLockFile(File, Error))
            {
                ErrorMessages.Add(FString::Printf(TEXT("Failed to force lock %s: %s"), *FPaths::GetCleanFilename(File), *Error));
                bAllSuccess = false;
            }
            else
            {
                // Successfully force-locked - make the file writable
                FCustomGitOperations::SetFileReadOnly(File, false);

                // Update status for this file
                FString CurrentUser = FCustomGitOperations::GetCurrentUserName();
                StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
            }
        }
        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
    }
    else if (Operation->GetName() == "Revert")
    {
        // Perforce-style Revert: Unlock the file, discard changes, make read-only
        bool bAllSuccess = true;

        // First, unload packages to release file handles
        FCustomGitOperations::UnloadPackagesForFiles(Files);

        for (const FString &File : Files)
        {
            FString Error;

            // First, unlock the file if locked
            FCustomGitOperations::UnlockFile(File, Error);

            // Then revert changes (git checkout)
            if (!FCustomGitOperations::RevertFile(File, Error))
            {
                // If revert fails, it might be an untracked file or already clean
                // Log but don't fail
                UE_LOG(LogTemp, Warning, TEXT("Revert warning for %s: %s"), *File, *Error);
            }

            // Make file read-only (Perforce-style)
            FCustomGitOperations::SetFileReadOnly(File, true);
        }
        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
    }
    else if (Operation->GetName() == "CheckIn")
    {
        // Perforce-style Check In: Commit files, unlock them, make read-only
        TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOp = StaticCastSharedRef<FCheckIn>(Operation);
        FString Msg = CheckInOp->GetDescription().ToString();

        bool bAllSuccess = true;
        FString Error;

        // Commit files
        if (!FCustomGitOperations::Commit(Msg, Files, Error))
        {
            ErrorMessages.Add(Error);
            bAllSuccess = false;
        }
        else
        {
            // Commit succeeded - unlock all files and make them read-only
            for (const FString &File : Files)
            {
                // Unlock the file
                FString UnlockError;
                FCustomGitOperations::UnlockFile(File, UnlockError);

                // Make file read-only (Perforce-style)
                FCustomGitOperations::SetFileReadOnly(File, true);
            }
        }

        // Auto-Push after successful commit
        if (bAllSuccess)
        {
            FString PushError;
            if (!FCustomGitOperations::Push(PushError))
            {
                // Push failed - warn but don't fail the check-in
                UE_LOG(LogTemp, Warning, TEXT("Auto-push failed: %s"), *PushError);
            }
        }

        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
    }
    else if (Operation->GetName() == "Sync")
    {
        FString Error;
        if (FCustomGitOperations::Pull(Error))
        {
            Result = ECommandResult::Succeeded;
        }
        else
        {
            Result = ECommandResult::Failed;
            ErrorMessages.Add(Error);
        }
    }
    else
    {
        // Unknown operation
        Result = ECommandResult::Failed;
    }

    return 0;
}

void FCustomGitSourceControlCommand::Stop()
{
}

void FCustomGitSourceControlCommand::Exit()
{
    // Dispatch the result back to the game thread
    // We captured 'this' pointer, so we must be careful not to delete before this lambda executes if it were not auto-delete.
    // But since we delete IN the lambda, it's fine.

    // Copy the results locally for the lambda caption if needed,
    // or just capture 'this' since we are inside Exit() which is called by the thread
    // and we need to dispatch to GameThread.

    // Actually, 'this' is alive until we delete it.

    TMap<FString, FString> CapturedStatusResults = StatusResults;

    AsyncTask(ENamedThreads::GameThread, [this, CapturedStatusResults]()
              {
                  // Update provider cache if we have status results
                  if (CapturedStatusResults.Num() > 0)
                  {
                      FCustomGitModule::Get().GetProvider().UpdateStateCache(CapturedStatusResults);
                  }

                  OperationCompleteDelegate.ExecuteIfBound(Operation, Result);
                  delete this; // Auto-delete on completion
              });
}
