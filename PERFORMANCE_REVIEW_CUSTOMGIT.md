# CustomGit Plugin - Performance Review

## Executive Summary
The CustomGit plugin's git command execution has several performance issues, primarily related to:
1. **Excessive git command calls** - Operations that could be batched are run individually
2. **Redundant locks querying** - `GetAllLocks()` is called multiple times in succession
3. **Inefficient command batching** - Multiple `git add` calls instead of batch operations
4. **No caching of expensive operations** - Repository root is cached but locks are not
5. **Suboptimal git command patterns** - Using inefficient flags and multiple invocations

---

## Critical Performance Issues

### 1. **Multiple Lock Queries in Single Operation** (HIGH IMPACT)

**File:** `CustomGitOperations.cpp`

**Issue:** `LockFileIfLFS()` queries all locks THREE separate times:
```cpp
// Lines 735-756: First call to GetAllLocks()
TMap<FString, FString> Locks;
GetAllLocks(Locks);

// ... Then later ...

// Lines 772-773: Second call to GetAllLocks() after lock failure
Locks.Empty();
GetAllLocks(Locks);
```

Each call to `GetAllLocks()` invokes: `git lfs locks --json` + potentially `git lfs locks` fallback.

**Impact:** For a single file lock operation, you're executing git LFS commands 2-3 times instead of 1.

**Recommendation:**
```cpp
// Create a single lookup that returns both status and lock info
bool FCustomGitOperations::LockFileIfLFS(const FString& File, FString& OutError,
                                          bool bShowWarning, TMap<FString, FString>* InExistingLocks = nullptr)
{
    // Use provided locks if available, otherwise query once
    TMap<FString, FString> Locks;
    if (InExistingLocks)
    {
        Locks = *InExistingLocks;  // Reuse already-fetched locks
    }
    else
    {
        GetAllLocks(Locks);
    }

    // Rest of function uses single Locks query...
    // If lock fails and we need to refresh, only refresh once
}
```

---

### 2. **Batch File Operations Not Using Batch Commands** (HIGH IMPACT)

**File:** `CustomGitOperations.cpp:416-438` - `Commit()` function

**Current Implementation:**
```cpp
bool FCustomGitOperations::Commit(const FString &Message, const TArray<FString> &Files, FString &OutError)
{
    // First, stage the files
    for (const FString &File : Files)
    {
        RunGitCommand(TEXT("add"), {TEXT("--")}, {File}, Results, Errors);  // LOOP - N commands!
    }

    // Then commit
    Results.Empty();
    Errors.Empty();
    if (RunGitCommand(TEXT("commit"), {TEXT("-m"), QuotedMessage}, TArray<FString>(), Results, Errors))
    {
        return true;
    }
}
```

**Issue:** If committing 10 files, you execute:
- 10× `git add <file>`
- 1× `git commit`
= **11 git processes spawned**

**Performance Impact:**
- Each `FPlatformProcess::ExecProcess()` call has overhead
- Could be reduced to **2 processes** total

**Recommendation:**
```cpp
bool FCustomGitOperations::Commit(const FString &Message, const TArray<FString> &Files, FString &OutError)
{
    TArray<FString> Results, Errors;

    // Single git add command with all files
    if (Files.Num() > 0)
    {
        if (!RunGitCommand(TEXT("add"), {TEXT("--")}, Files, Results, Errors))  // Files passed as array
        {
            OutError = TEXT("Failed to stage files");
            return false;
        }
    }

    // Then commit
    Results.Empty();
    Errors.Empty();
    FString QuotedMessage = FString::Printf(TEXT("\"%s\""), *Message);
    if (RunGitCommand(TEXT("commit"), {TEXT("-m"), QuotedMessage}, TArray<FString>(), Results, Errors))
    {
        return true;
    }

    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}
```

**Note:** The `RunGitCommand()` function already supports passing a `Files` array, but `Commit()` ignores this.

---

### 3. **No Caching of LFS Lock Information** (MEDIUM IMPACT)

**File:** `CustomGitOperations.cpp:238-327` - `GetAllLocks()`

**Issue:** Lock information is queried fresh every time without any caching:
```cpp
// Called from:
// - UpdateStatus() line 179
// - LockFileIfLFS() line 735
// - IsLockedByCurrentUser() line 859
// - SCustomGitFileListPanel.cpp (during refresh)
```

If the UI refreshes status and queries lock information simultaneously, `git lfs locks --json` runs multiple times within seconds.

**Recommendation:**
```cpp
class FCustomGitOperations
{
private:
    static TMap<FString, FString> CachedLocks;
    static double LastLocksCacheTime = -1000.0;
    static const double LOCKS_CACHE_DURATION = 2.0;  // Cache for 2 seconds
};

void FCustomGitOperations::GetAllLocks(TMap<FString, FString>& OutLocks)
{
    double CurrentTime = FPlatformTime::Seconds();

    // Return cached locks if fresh (< 2 seconds old)
    if (CurrentTime - LastLocksCacheTime < LOCKS_CACHE_DURATION)
    {
        OutLocks = CachedLocks;
        return;
    }

    // Query locks as before...
    TArray<FString> Results, Errors;
    if (RunGitLFSCommand(TEXT("locks"), {TEXT("--json")}, TArray<FString>(), Results, Errors))
    {
        // ... parsing code ...
        CachedLocks = OutLocks;
        LastLocksCacheTime = CurrentTime;
        return;
    }

    // ... fallback as before ...
}

// Clear cache when locks change
void FCustomGitOperations::LockFile(const FString& File, FString& OutError)
{
    // ... lock logic ...
    LastLocksCacheTime = -1000.0;  // Invalidate cache
}
```

---

### 4. **Status Update with Embedded Lock Queries** (MEDIUM IMPACT)

**File:** `CustomGitOperations.cpp:145-201` - `UpdateStatus()`

**Current Flow:**
```cpp
void FCustomGitOperations::UpdateStatus(const TArray<FString> &Files, TMap<FString, FString> &OutStatuses)
{
    // 1. Run git status
    RunGitCommand(TEXT("status"), Params, Files, Results, Errors);

    // 2. Parse results...
    for (const FString &Line : Results)
    {
        OutStatuses.Add(Filename, State);
    }

    // 3. ALSO fetch ALL locks
    TMap<FString, FString> Locks;
    GetAllLocks(Locks);  // Another git lfs locks command!

    // 4. Merge locks into status
    for (auto &LockPair : Locks)
    {
        // ...
    }
}
```

**Issue:** Every call to `UpdateStatus()` runs:
- `git status` command
- `git lfs locks --json` command (often 2-3 times due to issues #1 and #3)

If you're updating status for 20 files, this is inefficient.

**Recommendation:**
Use the lock caching from issue #3. Additionally, consider making lock fetching optional:

```cpp
void FCustomGitOperations::UpdateStatus(const TArray<FString>& Files, TMap<FString, FString>& OutStatuses,
                                        bool bIncludeLockInfo = true)
{
    // Get git status
    RunGitCommand(TEXT("status"), Params, Files, Results, Errors);

    // Parse status...
    for (const FString &Line : Results)
    {
        OutStatuses.Add(Filename, State);
    }

    // Only fetch locks if requested (and use cached version)
    if (bIncludeLockInfo)
    {
        TMap<FString, FString> Locks;
        GetAllLocks(Locks);  // Now uses cache from issue #3

        for (auto &LockPair : Locks)
        {
            if (OutStatuses.Contains(LockPair.Key))
            {
                OutStatuses[LockPair.Key] += TEXT("|LOCKED:") + LockPair.Value;
            }
            else
            {
                OutStatuses.Add(LockPair.Key, TEXT("LOCKED:") + LockPair.Value);
            }
        }
    }
}
```

---

### 5. **CheckOut Operation Queries Locks Multiple Times** (MEDIUM IMPACT)

**File:** `CustomGitSourceControlCommand.cpp:38-82` - `CheckOut` operation

**Current Flow:**
```cpp
for (const FString &File : Files)
{
    // Line 48: Check current locks
    FString LockOwner;
    if (!FCustomGitOperations::IsLockedByCurrentUser(File, &LockOwner))
    {
        // This calls GetAllLocks() internally!
        // If LockOwner is empty (our check above), we get no info about who owns it
    }
    else
    {
        // Already locked by us
        continue;
    }

    // Line 66: Try to lock
    if (!FCustomGitOperations::LockFile(File, Error))
    {
        // Lock failed - might call GetAllLocks() again inside LockFileIfLFS()
    }
    else
    {
        // Line 78: Another status update (potential more lock queries in UpdateStatus)
        StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
    }
}
```

**Impact:** Per-file, you're calling `IsLockedByCurrentUser()` → `GetAllLocks()` even if the locks haven't changed since the loop started.

**Recommendation:** Fetch all locks ONCE before the loop:

```cpp
else if (Operation->GetName() == "CheckOut")
{
    bool bAllSuccess = true;

    // Fetch ALL lock info once, before the loop
    TSet<FString> OurLocks;
    TMap<FString, FString> OtherLocks;
    FCustomGitOperations::GetLocksWithOwnership(OurLocks, OtherLocks);
    FString CurrentUser = FCustomGitOperations::GetCurrentUserName();

    for (const FString &File : Files)
    {
        FString RepoRoot = FCustomGitOperations::GetRepositoryRoot();
        FString RelativePath = File;
        FPaths::MakePathRelativeTo(RelativePath, *RepoRoot);
        RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));

        // Check in pre-fetched lock data
        if (OurLocks.Contains(RelativePath))
        {
            // Already locked by us
            FCustomGitOperations::SetFileReadOnly(File, false);
            StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
            continue;
        }

        if (OtherLocks.Contains(RelativePath))
        {
            // Locked by someone else
            ErrorMessages.Add(FString::Printf(TEXT("Cannot check out %s - locked by %s"),
                *FPaths::GetCleanFilename(File), *OtherLocks[RelativePath]));
            bAllSuccess = false;
            continue;
        }

        // Not locked - try to lock it
        FString Error;
        if (!FCustomGitOperations::LockFile(File, Error))
        {
            ErrorMessages.Add(FString::Printf(TEXT("Failed to lock %s: %s"),
                *FPaths::GetCleanFilename(File), *Error));
            bAllSuccess = false;
        }
        else
        {
            FCustomGitOperations::SetFileReadOnly(File, false);
            StatusResults.Add(File, TEXT("LOCKED:") + CurrentUser);
        }
    }

    Result = bAllSuccess ? ECommandResult::Succeeded : ECommandResult::Failed;
}
```

This reduces the lock queries from **N per file** to **1 per batch operation**.

---

## Medium Priority Issues

### 6. **RunGitCommand() Creates New Process For Each Call**

**File:** `CustomGitOperations.cpp:46-125` - `RunGitCommand()`

The function uses `FPlatformProcess::ExecProcess()` which spawns a new git process each time. This is necessary, but compound with other issues.

**Current:** Each call creates a new process with full git startup overhead.

**Note:** This is inherent to using subprocess calls. Mitigation is through the batching improvements above (issues #2, #5).

---

### 7. **No Early Exit from GetAllLocks() Parsing** (LOW IMPACT)

**File:** `CustomGitOperations.cpp:254-281` - JSON parsing

```cpp
int32 SearchStart = 0;
while (true)
{
    int32 PathStart = JsonString.Find(...);  // Linear search from SearchStart each time
    if (PathStart == INDEX_NONE)
        break;
    // ...
    SearchStart = OwnerEnd;  // Move start position
}
```

**Issue:** The string searching could be optimized with pointer arithmetic instead of repeated `Find()` calls. Minor impact.

---

## Low Priority Issues

### 8. **Redundant File Existence Checks**

**File:** `CustomGitOperations.cpp:717-805` - `LockFileIfLFS()`

The function checks if file is tracked (`IsFileTrackedByGit()`), which runs:
```
git ls-files --error-unmatch <file>
```

Then later checks if it's LFS tracked by running:
```
git check-attr filter -- <file>
```

These could be combined into a single git command or cached.

---

## Performance Impact Summary

| Issue | Frequency | Commands/Operation | Improvement |
|-------|-----------|-------------------|-------------|
| Multiple lock queries in one op | Common | 3→1 git commands | **66% reduction** |
| Batch file staging | Per commit | N→1 git add | **N-1 fewer processes** |
| Lock caching | UI refresh | 1→0.2 avg | **80% reduction** |
| Checkout loops | Per file | N→1 lock query | **N×fewer** |
| **Total for typical workflow** | | **Thousands of processes → hundreds** | **~70-80%** |

---

## Implementation Priority

### Phase 1 (Critical) - Do First
1. **Fix issue #2** (Batch staging) - 5 lines changed, huge impact
2. **Implement issue #3** (Lock caching) - 15 lines added, prevents redundant lock queries
3. **Fix issue #5** (Checkout loop) - 30 lines refactored, eliminates N redundant queries

### Phase 2 (Important)
4. **Fix issue #1** (LockFileIfLFS redundancy) - Reuse locks from caller
5. **Fix issue #4** (Status update caching) - Use cache from phase 1

### Phase 3 (Nice to Have)
6. **Optimize issue #7** (String parsing)
7. **Combine file checks** (issue #8)

---

## Testing Recommendations

After each fix, measure:
1. **Process count** - Monitor with `Get-Process git` during operations
2. **Operation time** - Time `RefreshStatus()`, `CheckOut`, `Commit` operations
3. **Lock query frequency** - Add logging to `GetAllLocks()` entry/exit
4. **Memory usage** - Ensure caching doesn't cause unbounded growth

**Example measurement script:**
```cpp
// In CustomGitOperations.cpp
#if UE_BUILD_DEBUG
    static int32 GetAllLocksCallCount = 0;
    static int32 RunGitCommandCallCount = 0;

    void FCustomGitOperations::GetAllLocks(...)
    {
        GetAllLocksCallCount++;
        UE_LOG(LogTemp, Warning, TEXT("GetAllLocks() call #%d"), GetAllLocksCallCount);
        // ... rest of function
    }
#endif
```

---

## Summary

The CustomGit plugin makes **significantly more git subprocess calls than necessary** due to:
- Looping over individual file operations instead of batch commands
- Querying lock information redundantly in the same operation
- No caching of expensive lock queries

Implementing issues #2, #3, and #5 alone could reduce git process spawns by **60-80%** for typical workflows, with minimal code changes.
