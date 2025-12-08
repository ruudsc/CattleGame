#include "CustomGitSourceControlCommand.h"
#include "HAL/RunnableThread.h"
#include "CustomGitOperations.h"
#include "CustomGitModule.h"
#include "CustomGitSourceControlProvider.h"
#include "Async/Async.h"
#include "SourceControlOperations.h"

FCustomGitSourceControlCommand::FCustomGitSourceControlCommand(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation, const TArray<FString>& InFiles, const FSourceControlOperationComplete& InOperationCompleteDelegate)
    : Operation(InOperation)
    , Files(InFiles)
    , OperationCompleteDelegate(InOperationCompleteDelegate)
    , Result(ECommandResult::Failed)
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
        bool bAllSuccess = true;
        for (const FString& File : Files)
        {
            FString Error;
            if (!FCustomGitOperations::LockFile(File, Error))
            {
                ErrorMessages.Add(FString::Printf(TEXT("Failed to lock %s: %s"), *File, *Error));
                bAllSuccess = false;
            }
        }
        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
        // Update status for these files?
        // Typically we want to refresh status after operation.
        // We can create a partial status map here.
        // For simplicity, let's trust the operation and maybe force a status update later.
        // Or manually update the map passed back to Provider.
        // For CheckOut, we know we locked them.
    }
    else if (Operation->GetName() == "Revert")
    {
        bool bAllSuccess = true;
        for (const FString& File : Files)
        {
            FString Error;
            if (!FCustomGitOperations::RevertFile(File, Error))
            {
                ErrorMessages.Add(FString::Printf(TEXT("Failed to revert %s: %s"), *File, *Error));
                bAllSuccess = false;
            }
        }
        Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
    }
    else if (Operation->GetName() == "CheckIn")
    {
        TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOp = StaticCastSharedRef<FCheckIn>(Operation);
        FString Msg = CheckInOp->GetDescription().ToString();
        
        bool bAllSuccess = true;
        // Commit files
        FString Error;
        if (!FCustomGitOperations::Commit(Msg, Files, Error))
        {
             ErrorMessages.Add(Error);
             bAllSuccess = false;
        }
        
        // Auto-Push?
        // If success, push?
        // FCustomGitOperations::Push(Error);
        
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
