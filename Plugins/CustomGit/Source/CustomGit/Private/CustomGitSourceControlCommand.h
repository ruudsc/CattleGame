#pragma once

#include "CoreMinimal.h"
#include "ISourceControlProvider.h"
#include "HAL/Runnable.h"

class FCustomGitSourceControlCommand : public FRunnable
{
public:
    FCustomGitSourceControlCommand(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation, const TArray<FString>& InFiles, const FSourceControlOperationComplete& InOperationCompleteDelegate);

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

private:
    /** The source control operation we are performing */
    TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> Operation;

    /** The files to operate on */
    TArray<FString> Files;

    /** The delegate to call when the operation is complete */
    FSourceControlOperationComplete OperationCompleteDelegate;

    /** Errors and messages from the operation */
    TArray<FString> ErrorMessages;
    TArray<FString> InfoMessages;

    /** Result of the operation */
    ECommandResult::Type Result;

    /** Map of filenames to status info, populated by UpdateStatus operation */
    TMap<FString, FString> StatusResults;
};
