#pragma once

#include "CoreMinimal.h"
#include "ISourceControlProvider.h"

class FCustomGitSourceControlProvider : public ISourceControlProvider
{
public:
    /** ISourceControlProvider implementation */
    virtual void Init(bool bForceConnection = true) override;
    virtual void Close() override;
    virtual FText GetStatusText() const override;
    virtual TMap<EStatus, FString> GetStatus() const override;
    virtual bool IsEnabled() const override;
    virtual bool IsAvailable() const override;
    virtual const FName& GetName(void) const override;
    virtual bool QueryStateBranchConfig(const FString& ConfigSrc, const FString& ConfigDest) override;
    
    virtual void RegisterStateBranches(const TArray<FString>& BranchNames, const FString& ContentRoot) override;
    virtual int32 GetStateBranchIndex(const FString& BranchName) const override;
    virtual bool GetStateBranchAtIndex(int32 BranchIndex, FString& OutBranchName) const override;

    virtual ECommandResult::Type GetState( const TArray<FString>& InFiles, TArray<FSourceControlStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage ) override;
    virtual ECommandResult::Type GetState( const TArray<UPackage*>& InPackages, TArray<FSourceControlStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage ) override;
    virtual FSourceControlStatePtr GetState( const UPackage* InPackage, EStateCacheUsage::Type InStateCacheUsage ) override;
    virtual FSourceControlChangelistStatePtr GetState( const FSourceControlChangelistRef& InChangelist, EStateCacheUsage::Type InStateCacheUsage ) override;
    virtual ECommandResult::Type GetState( const TArray<FSourceControlChangelistRef>& InChangelists, TArray<FSourceControlChangelistStateRef>& OutState, EStateCacheUsage::Type InStateCacheUsage ) override;
    virtual TArray<FSourceControlStateRef> GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef&)> Predicate) const override;

    virtual FDelegateHandle RegisterSourceControlStateChanged_Handle( const FSourceControlStateChanged::FDelegate& SourceControlStateChanged ) override;
    virtual void UnregisterSourceControlStateChanged_Handle( FDelegateHandle Handle ) override;

    virtual ECommandResult::Type Execute( const FSourceControlOperationRef& InOperation, FSourceControlChangelistPtr InChangelist, const TArray<FString>& InFiles, EConcurrency::Type InConcurrency = EConcurrency::Synchronous, const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete() ) override;
    
    virtual bool CanExecuteOperation( const FSourceControlOperationRef& InOperation ) const override;
    virtual bool CanCancelOperation( const FSourceControlOperationRef& InOperation ) const override;
    virtual void CancelOperation( const FSourceControlOperationRef& InOperation ) override;

    virtual bool UsesLocalReadOnlyState() const override;
    virtual bool UsesChangelists() const override;
    virtual bool UsesUncontrolledChangelists() const override;
    virtual bool UsesCheckout() const override;
    virtual bool UsesFileRevisions() const override;
    virtual bool UsesSnapshots() const override;
    virtual bool AllowsDiffAgainstDepot() const override;

    virtual TOptional<bool> IsAtLatestRevision() const override;
    virtual TOptional<int> GetNumLocalChanges() const override;
    
    virtual void Tick() override;
    virtual TArray< TSharedRef<class ISourceControlLabel> > GetLabels( const FString& InMatchingSpec ) const override;
    virtual TArray<FSourceControlChangelistRef> GetChangelists(EStateCacheUsage::Type InStateCacheUsage) override;
    virtual TSharedRef<class SWidget> MakeSettingsWidget() const override;

    // Deprecated?
    virtual TSharedPtr<ISourceControlState, ESPMode::ThreadSafe> GetState(const FString& Filename, EStateCacheUsage::Type CacheUsage) override;

    /** Updates the state cache with new statuses */
    void UpdateStateCache(const TMap<FString, FString>& Statuses);

private:
    /** Cache of file states */
    TMap<FString, TSharedRef<class FCustomGitSourceControlState, ESPMode::ThreadSafe>> StateCache;
    
    FSourceControlStateChanged OnSourceControlStateChanged;
};
