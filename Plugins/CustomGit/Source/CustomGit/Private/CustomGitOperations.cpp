#include "CustomGitOperations.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"

TArray<FString> FCustomGitOperations::CommandHistory;
FString FCustomGitOperations::RepositoryRoot;
FString FCustomGitOperations::CachedUserName;

// Lock cache static variables (Issue #3 from performance review)
TMap<FString, FString> FCustomGitOperations::CachedLocks;
TSet<FString> FCustomGitOperations::CachedOurLocks;
TMap<FString, FString> FCustomGitOperations::CachedOtherLocks;
double FCustomGitOperations::LastLocksCacheTime = -1000.0;
double FCustomGitOperations::LastOwnershipCacheTime = -1000.0;

FString FCustomGitOperations::GetRepositoryRoot()
{
    if (RepositoryRoot.IsEmpty())
    {
        // Try to find the repository root from the project directory
        FString ProjectDir = FPaths::ProjectDir();
        FPaths::NormalizeDirectoryName(ProjectDir);

        // Check if we're in a git repo by looking for .git folder
        FString CurrentDir = ProjectDir;
        while (!CurrentDir.IsEmpty())
        {
            FString GitDir = FPaths::Combine(CurrentDir, TEXT(".git"));
            if (IFileManager::Get().DirectoryExists(*GitDir))
            {
                RepositoryRoot = CurrentDir;
                break;
            }
            // Go up one directory
            FString ParentDir = FPaths::GetPath(CurrentDir);
            if (ParentDir == CurrentDir)
            {
                break; // Reached root
            }
            CurrentDir = ParentDir;
        }

        // Fallback to project directory if no .git found
        if (RepositoryRoot.IsEmpty())
        {
            RepositoryRoot = ProjectDir;
        }
    }
    return RepositoryRoot;
}

bool FCustomGitOperations::RunGitCommand(const FString &Command, const TArray<FString> &Parameters, const TArray<FString> &Files, TArray<FString> &OutResults, TArray<FString> &OutErrors)
{
    auto QuoteArg = [](const FString &Arg) -> FString
    {
        const FString Trimmed = Arg.TrimStartAndEnd();
        if (Trimmed.Len() >= 2 && Trimmed.StartsWith(TEXT("\"")) && Trimmed.EndsWith(TEXT("\"")))
        {
            return Trimmed;
        }

        const bool bNeedsQuoting = Trimmed.Contains(TEXT(" ")) || Trimmed.Contains(TEXT("\t")) || Trimmed.Contains(TEXT("\n")) || Trimmed.Contains(TEXT("\r"));
        if (!bNeedsQuoting && !Trimmed.Contains(TEXT("\"")))
        {
            return Trimmed;
        }

        FString Escaped = Trimmed;
        Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
        return FString::Printf(TEXT("\"%s\""), *Escaped);
    };

    FString GitBinary = TEXT("git"); // Should be configurable
    FString RepoRoot = GetRepositoryRoot();

    // Build the display command for history (without -C path)
    FString DisplayCommand = FString::Printf(TEXT("git %s"), *Command);
    for (const FString &Param : Parameters)
    {
        DisplayCommand += TEXT(" ");
        DisplayCommand += QuoteArg(Param);
    }
    for (const FString &File : Files)
    {
        DisplayCommand += TEXT(" \"");
        DisplayCommand += File;
        DisplayCommand += TEXT("\"");
    }
    AddToHistory(DisplayCommand);

    // Use -C flag to run git from the repository root
    FString CommandLine = FString::Printf(TEXT("-C \"%s\" %s"), *RepoRoot, *Command);

    for (const FString &Param : Parameters)
    {
        CommandLine += TEXT(" ");
        CommandLine += QuoteArg(Param);
    }

    for (const FString &File : Files)
    {
        CommandLine += TEXT(" \"");
        CommandLine += File;
        CommandLine += TEXT("\"");
    }

    // This is a simplified implementation. In reality, we need pipe handling.
    FString StdOut, StdErr;
    int32 ReturnCode;
    FPlatformProcess::ExecProcess(*GitBinary, *CommandLine, &ReturnCode, &StdOut, &StdErr);

    if (!StdOut.IsEmpty())
    {
        StdOut.ParseIntoArrayLines(OutResults);
    }
    if (!StdErr.IsEmpty())
    {
        StdErr.ParseIntoArrayLines(OutErrors);
    }

    if (ReturnCode != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CustomGit: git command failed (code %d): %s"), ReturnCode, *DisplayCommand);
        if (!StdErr.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("CustomGit: git stderr: %s"), *StdErr);
        }
    }

    return ReturnCode == 0;
}

bool FCustomGitOperations::RunGitLFSCommand(const FString &Command, const TArray<FString> &Parameters, const TArray<FString> &Files, TArray<FString> &OutResults, TArray<FString> &OutErrors)
{
    // Simply delegate to normal git command with "lfs" as first param?
    // Or run "git-lfs" directly? Usually "git lfs".
    TArray<FString> NewParams;
    NewParams.Add(TEXT("lfs"));
    NewParams.Add(Command);
    NewParams.Append(Parameters);

    return RunGitCommand("", NewParams, Files, OutResults, OutErrors); // Command empty because we pass args manually
}

bool FCustomGitOperations::CheckGitAvailability()
{
    TArray<FString> Results, Errors;
    return RunGitCommand(TEXT("version"), TArray<FString>(), TArray<FString>(), Results, Errors);
}

void FCustomGitOperations::UpdateStatus(const TArray<FString> &Files, TMap<FString, FString> &OutStatuses)
{
    TArray<FString> Results, Errors;
    TArray<FString> Params;
    Params.Add(TEXT("--porcelain"));

    // logic to run on specific files or all
    // If files are provided, append them.
    // Note: git status on specific files updates them.

    RunGitCommand(TEXT("status"), Params, Files, Results, Errors);

    for (const FString &Line : Results)
    {
        if (Line.Len() > 3)
        {
            FString State = Line.Left(2);
            FString Filename = Line.RightChop(3);
            // Handle quotes in filename if any
            Filename = Filename.TrimQuotes();
            // Convert to absolute path if needed, but git returns relative to root usually.
            // UE expects absolute paths usually.
            // We need to resolve path.
            // For now, assume simple relative paths.
            OutStatuses.Add(Filename, State);
        }
    }

    // Also fetch locks if we are doing a broad update (e.g. no files specified or many files)
    // For performance, maybe only if requested? But UE expects lock state.
    // Let's separate GetAllLocks call or call it here.
    // Ideally, we merge it.

    TMap<FString, FString> Locks;
    GetAllLocks(Locks);
    // Merge locks into status
    // Use a special key or just implies "Locked" state?
    // The Provider State Cache needs to know if it's locked.
    // But OutStatuses is just a string map.
    // We should probably encoding it like "M|Locked" or handle it in Provider update.
    // For now, let's keep it separate or just let Provider calling both.
    // BUT CustomGitSourceControlCommand runs UpdateStatus.
    // Better: UpdateStatus runs both and returns combined or we add a separate method in logic.
    // Modifying UpdateStatus to merge locks:
    for (auto &LockPair : Locks)
    {
        // If file already in status, append lock info?
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

bool FCustomGitOperations::LockFile(const FString &File, FString &OutError)
{
    TArray<FString> Results, Errors;
    // git lfs lock "file"

    if (RunGitLFSCommand(TEXT("lock"), TArray<FString>(), {File}, Results, Errors))
    {
        InvalidateLockCache(); // Cache is now stale
        return true;
    }

    if (Errors.Num() > 0)
    {
        OutError = Errors[0];
    }
    return false;
}

bool FCustomGitOperations::UnlockFile(const FString &File, FString &OutError)
{
    TArray<FString> Results, Errors;
    // git lfs unlock "file"
    if (RunGitLFSCommand(TEXT("unlock"), TArray<FString>(), {File}, Results, Errors))
    {
        InvalidateLockCache(); // Cache is now stale
        return true;
    }

    if (Errors.Num() > 0)
    {
        OutError = Errors[0];
    }
    return false;
}

void FCustomGitOperations::GetAllLocks(TMap<FString, FString> &OutLocks)
{
    // Check cache first (Issue #3 from performance review)
    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastLocksCacheTime < LOCKS_CACHE_DURATION)
    {
        OutLocks = CachedLocks;
        return;
    }

    TArray<FString> Results, Errors;

    // Try with --json first for more reliable parsing
    if (RunGitLFSCommand(TEXT("locks"), {TEXT("--json")}, TArray<FString>(), Results, Errors))
    {
        FString JsonString = FString::Join(Results, TEXT("\n"));

        // Simple JSON parsing without full JSON library
        // Format: [{"path":"file.uasset","owner":{"name":"User Name"},"id":"123"},...]
        // Find all "path":"..." and "name":"..." pairs

        int32 SearchStart = 0;
        while (true)
        {
            int32 PathStart = JsonString.Find(TEXT("\"path\":\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchStart);
            if (PathStart == INDEX_NONE)
                break;

            PathStart += 8; // Skip "path":"
            int32 PathEnd = JsonString.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, PathStart);
            if (PathEnd == INDEX_NONE)
                break;

            FString Path = JsonString.Mid(PathStart, PathEnd - PathStart);

            // Find the owner name after this path
            int32 OwnerStart = JsonString.Find(TEXT("\"name\":\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, PathEnd);
            if (OwnerStart != INDEX_NONE)
            {
                OwnerStart += 8; // Skip "name":"
                int32 OwnerEnd = JsonString.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, OwnerStart);
                if (OwnerEnd != INDEX_NONE)
                {
                    FString Owner = JsonString.Mid(OwnerStart, OwnerEnd - OwnerStart);
                    OutLocks.Add(Path, Owner);
                    SearchStart = OwnerEnd;
                    continue;
                }
            }

            SearchStart = PathEnd;
        }

        UE_LOG(LogTemp, Log, TEXT("CustomGit: Found %d LFS locks"), OutLocks.Num());

        // Update cache
        CachedLocks = OutLocks;
        LastLocksCacheTime = CurrentTime;
        return;
    }

    // Fallback to standard output parsing
    if (RunGitLFSCommand(TEXT("locks"), TArray<FString>(), TArray<FString>(), Results, Errors))
    {
        // Standard output format: "path/to/file\tUser Name\tID:123"
        // Or: "path/to/file  User Name  ID:123" (tab or spaces)
        for (const FString &Line : Results)
        {
            if (Line.IsEmpty())
                continue;

            // Try to find ID: marker to help parse
            int32 IdIndex = Line.Find(TEXT("ID:"));
            if (IdIndex != INDEX_NONE)
            {
                // Try tab separation first
                TArray<FString> Parts;
                Line.ParseIntoArray(Parts, TEXT("\t"), true);

                if (Parts.Num() >= 2)
                {
                    OutLocks.Add(Parts[0].TrimStartAndEnd(), Parts[1].TrimStartAndEnd());
                }
                else
                {
                    // Fallback: assume first token is the path (files shouldn't have spaces in Unreal)
                    FString TrimmedLine = Line.TrimStartAndEnd();
                    int32 FirstSpace = TrimmedLine.Find(TEXT(" "));
                    if (FirstSpace != INDEX_NONE)
                    {
                        FString Path = TrimmedLine.Left(FirstSpace);
                        // Get everything between first space and "ID:"
                        FString Owner = TrimmedLine.Mid(FirstSpace + 1, IdIndex - FirstSpace - 1).TrimStartAndEnd();
                        OutLocks.Add(Path, Owner);
                    }
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("CustomGit: Found %d LFS locks (standard output)"), OutLocks.Num());

        // Update cache
        CachedLocks = OutLocks;
        LastLocksCacheTime = CurrentTime;
    }
}

void FCustomGitOperations::InvalidateLockCache()
{
    LastLocksCacheTime = -1000.0;
    LastOwnershipCacheTime = -1000.0;
    CachedLocks.Empty();
    CachedOurLocks.Empty();
    CachedOtherLocks.Empty();
}

void FCustomGitOperations::GetLocksWithOwnership(TSet<FString> &OutOurLocks, TMap<FString, FString> &OutOtherLocks)
{
    // Check cache first (Issue #3 from performance review)
    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastOwnershipCacheTime < LOCKS_CACHE_DURATION)
    {
        OutOurLocks = CachedOurLocks;
        OutOtherLocks = CachedOtherLocks;
        return;
    }

    TArray<FString> Results, Errors;

    // Use --verify flag which marks our locks with "O" prefix
    // Output format: "O path/to/file\tUsername\tID:123" for our locks
    //                "  path/to/file\tUsername\tID:123" for others' locks
    if (RunGitLFSCommand(TEXT("locks"), {TEXT("--verify")}, TArray<FString>(), Results, Errors))
    {
        for (const FString &Line : Results)
        {
            if (Line.Len() < 2)
                continue;

            // First character indicates ownership: 'O' = ours, ' ' = theirs
            bool bIsOurs = (Line[0] == TEXT('O'));

            // Skip the ownership marker and any following whitespace
            FString TrimmedLine = Line.RightChop(1).TrimStart();

            // Find ID: marker to help parse
            int32 IdIndex = TrimmedLine.Find(TEXT("ID:"));
            if (IdIndex == INDEX_NONE)
                continue;

            // Parse: "path/to/file\tUsername\tID:123" or space-separated
            TArray<FString> Parts;
            TrimmedLine.ParseIntoArray(Parts, TEXT("\t"), true);

            FString FilePath;
            FString Owner;

            if (Parts.Num() >= 2)
            {
                FilePath = Parts[0].TrimStartAndEnd();
                Owner = Parts[1].TrimStartAndEnd();
            }
            else
            {
                // Fallback: space-separated
                int32 FirstSpace = TrimmedLine.Find(TEXT(" "));
                if (FirstSpace != INDEX_NONE)
                {
                    FilePath = TrimmedLine.Left(FirstSpace);
                    Owner = TrimmedLine.Mid(FirstSpace + 1, IdIndex - FirstSpace - 1).TrimStartAndEnd();
                }
            }

            if (!FilePath.IsEmpty())
            {
                if (bIsOurs)
                {
                    OutOurLocks.Add(FilePath);
                }
                else
                {
                    OutOtherLocks.Add(FilePath, Owner);
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("CustomGit: Found %d locks owned by us, %d by others"), OutOurLocks.Num(), OutOtherLocks.Num());

        // Update cache
        CachedOurLocks = OutOurLocks;
        CachedOtherLocks = OutOtherLocks;
        LastOwnershipCacheTime = CurrentTime;
    }
}

bool FCustomGitOperations::RevertFile(const FString &File, FString &OutError)
{
    // Revert means discarding local changes.
    // If locked, we should unlock? UE expects Revert to discard changes AND unlock if checked out.
    UnlockFile(File, OutError);

    // git checkout HEAD -- file
    TArray<FString> Results, Errors;
    // We use "HEAD" to revert to last commit, or just "--" which reverts to index?
    // Usually "checkout HEAD --" is safer to revert to last commit.
    // But if we have staged changes?
    // For safety, "checkout --" reverts to Index. "reset HEAD --" + "checkout --" reverts to HEAD.
    // Let's do simple checkout --
    if (RunGitCommand(TEXT("checkout"), {TEXT("--")}, {File}, Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

bool FCustomGitOperations::Commit(const FString &Message, const TArray<FString> &Files, FString &OutError)
{
    TArray<FString> Results, Errors;

    // Stage all files in a single batch command (Issue #2 from performance review)
    // This reduces N git processes to 1 for staging
    if (Files.Num() > 0)
    {
        if (!RunGitCommand(TEXT("add"), {TEXT("--")}, Files, Results, Errors))
        {
            if (Errors.Num() > 0)
            {
                OutError = TEXT("Failed to stage files: ") + Errors[0];
            }
            else
            {
                OutError = TEXT("Failed to stage files");
            }
            return false;
        }
    }

    // Then commit with the message (quote the message to handle spaces and special chars)
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

bool FCustomGitOperations::Push(FString &OutError)
{
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("push"), TArray<FString>(), TArray<FString>(), Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

bool FCustomGitOperations::Pull(FString &OutError)
{
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("pull"), TArray<FString>(), TArray<FString>(), Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

void FCustomGitOperations::GetUserInfo(FString &OutUserName, FString &OutEmail)
{
    TArray<FString> Results, Errors;

    // Get user.name
    if (RunGitCommand(TEXT("config"), {TEXT("user.name")}, TArray<FString>(), Results, Errors))
    {
        if (Results.Num() > 0)
        {
            OutUserName = Results[0].TrimStartAndEnd();
        }
    }

    // Get user.email
    Results.Empty();
    Errors.Empty();
    if (RunGitCommand(TEXT("config"), {TEXT("user.email")}, TArray<FString>(), Results, Errors))
    {
        if (Results.Num() > 0)
        {
            OutEmail = Results[0].TrimStartAndEnd();
        }
    }
}

void FCustomGitOperations::GetBranches(TArray<FGitBranchInfo> &OutBranches, FString &OutCurrentBranch)
{
    OutCurrentBranch.Empty();
    TArray<FString> Results, Errors;

    // Use -vv to get verbose output with upstream tracking info
    // Format: "* main    abc1234 [origin/main] Commit message" or
    //         "  feature abc1234 [origin/feature: gone] Commit message" or
    //         "  local   abc1234 Commit message" (no upstream)
    if (RunGitCommand(TEXT("branch"), {TEXT("-vv")}, TArray<FString>(), Results, Errors))
    {
        for (const FString &Line : Results)
        {
            FString TrimmedLine = Line.TrimStartAndEnd();
            if (TrimmedLine.IsEmpty())
            {
                continue;
            }

            FGitBranchInfo BranchInfo;

            // Check if this is the current branch (starts with "* ")
            if (TrimmedLine.StartsWith(TEXT("* ")))
            {
                BranchInfo.bIsCurrent = true;
                TrimmedLine = TrimmedLine.RightChop(2); // Remove "* "
            }

            // Parse the branch name (first word)
            int32 SpaceIndex;
            if (TrimmedLine.FindChar(TEXT(' '), SpaceIndex))
            {
                BranchInfo.Name = TrimmedLine.Left(SpaceIndex);
                FString Remainder = TrimmedLine.RightChop(SpaceIndex + 1).TrimStart();

                // Check for upstream tracking info in brackets
                // Look for [origin/xxx] or [origin/xxx: gone]
                int32 BracketStart, BracketEnd;
                if (Remainder.FindChar(TEXT('['), BracketStart) && Remainder.FindChar(TEXT(']'), BracketEnd))
                {
                    FString TrackingInfo = Remainder.Mid(BracketStart + 1, BracketEnd - BracketStart - 1);

                    // Check if upstream is gone
                    if (TrackingInfo.Contains(TEXT(": gone")))
                    {
                        BranchInfo.bIsUpstreamGone = true;
                    }
                    // Has tracking info but not gone = normal tracked branch
                    BranchInfo.bIsLocal = false;
                }
                else
                {
                    // No brackets means no upstream - local only branch
                    BranchInfo.bIsLocal = true;
                }
            }
            else
            {
                // Just the branch name, no other info
                BranchInfo.Name = TrimmedLine;
                BranchInfo.bIsLocal = true;
            }

            if (BranchInfo.bIsCurrent)
            {
                OutCurrentBranch = BranchInfo.Name;
            }

            OutBranches.Add(BranchInfo);
        }
    }
}

void FCustomGitOperations::GetCommitHistory(int32 MaxCount, TArray<FString> &OutCommits)
{
    TArray<FString> Results, Errors;
    FString MaxCountStr = FString::Printf(TEXT("-n%d"), MaxCount);
    // Format: abbreviated hash - subject
    if (RunGitCommand(TEXT("log"), {MaxCountStr, TEXT("--pretty=format:%h - %s (%cr)")}, TArray<FString>(), Results, Errors))
    {
        for (const FString &Line : Results)
        {
            if (!Line.IsEmpty())
            {
                OutCommits.Add(Line);
            }
        }
    }
}

bool FCustomGitOperations::CreateBranch(const FString &BranchName, FString &OutError)
{
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("branch"), {BranchName}, TArray<FString>(), Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

bool FCustomGitOperations::SwitchBranch(const FString &BranchName, FString &OutError)
{
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("checkout"), {BranchName}, TArray<FString>(), Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

bool FCustomGitOperations::Merge(const FString &BranchName, FString &OutError)
{
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("merge"), {BranchName}, TArray<FString>(), Results, Errors))
    {
        return true;
    }
    if (Errors.Num() > 0)
        OutError = Errors[0];
    return false;
}

#include "HAL/PlatformFileManager.h"

FDateTime FCustomGitOperations::GetFileLastModified(const FString &File)
{
    IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.GetTimeStamp(*File);
}

void FCustomGitOperations::AddToHistory(const FString &Command)
{
    // Keep internal history limit?
    if (CommandHistory.Num() > 100)
        CommandHistory.RemoveAt(0);
    CommandHistory.Add(FDateTime::Now().ToString(TEXT("%H:%M:%S")) + TEXT(": ") + Command);
}

const TArray<FString> &FCustomGitOperations::GetCommandHistory()
{
    return CommandHistory;
}

bool FCustomGitOperations::IsBinaryAsset(const FString &File)
{
    // Check if file has a binary asset extension
    FString Extension = FPaths::GetExtension(File, false).ToLower();

    // Common Unreal Engine binary asset extensions
    static const TArray<FString> BinaryExtensions = {
        TEXT("uasset"),
        TEXT("umap"),
        TEXT("ubulk"),
        TEXT("upk"),
        TEXT("png"),
        TEXT("jpg"),
        TEXT("jpeg"),
        TEXT("tga"),
        TEXT("bmp"),
        TEXT("psd"),
        TEXT("wav"),
        TEXT("mp3"),
        TEXT("ogg"),
        TEXT("fbx"),
        TEXT("obj"),
        TEXT("dae"),
        TEXT("abc"),
        TEXT("gltf"),
        TEXT("glb"),
        TEXT("mp4"),
        TEXT("mov"),
        TEXT("avi"),
        TEXT("ttf"),
        TEXT("otf"),
        TEXT("exr"),
        TEXT("hdr")};

    return BinaryExtensions.Contains(Extension);
}

bool FCustomGitOperations::IsLFSTrackedFile(const FString &File)
{
    // Check if file would be tracked by LFS by checking .gitattributes patterns
    // For simplicity, we check if it's a binary asset type which should be in LFS
    // More robust: run 'git check-attr filter -- <file>' and look for 'lfs'

    if (!IsBinaryAsset(File))
    {
        return false;
    }

    // Verify with git if this file is actually LFS tracked
    TArray<FString> Results, Errors;
    if (RunGitCommand(TEXT("check-attr"), {TEXT("filter"), TEXT("--")}, {File}, Results, Errors))
    {
        for (const FString &Line : Results)
        {
            if (Line.Contains(TEXT("lfs")))
            {
                return true;
            }
        }
    }

    // Fallback: if it's a binary asset in the Content folder, assume it should be LFS tracked
    return File.Contains(TEXT("Content/")) && IsBinaryAsset(File);
}

bool FCustomGitOperations::IsFileTrackedByGit(const FString &File)
{
    // Use 'git ls-files' to check if the file is tracked
    // A tracked file will appear in the output, an untracked file will not
    TArray<FString> Results, Errors;

    if (RunGitCommand(TEXT("ls-files"), {TEXT("--error-unmatch")}, {File}, Results, Errors))
    {
        // File is tracked (command succeeded)
        return true;
    }

    // Command failed - file is not tracked
    return false;
}

bool FCustomGitOperations::LockFileIfLFS(const FString &File, FString &OutError, bool bShowWarning)
{
    // First check if file is tracked by git at all
    if (!IsFileTrackedByGit(File))
    {
        // File is not tracked by git yet (new file), no need to lock
        UE_LOG(LogTemp, Verbose, TEXT("CustomGit: File '%s' is not tracked by git, skipping lock"), *File);
        return true;
    }

    // Check if file is tracked by LFS
    if (!IsLFSTrackedFile(File))
    {
        return true; // Not LFS tracked, nothing to do
    }

    // Get the relative path for comparison
    FString RepoRoot = GetRepositoryRoot();
    FString RelativePath = File;
    FPaths::MakePathRelativeTo(RelativePath, *RepoRoot);
    RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));

    // Issue #1 from performance review: Use GetLocksWithOwnership once instead of multiple GetAllLocks calls
    TSet<FString> OurLocks;
    TMap<FString, FString> OtherLocks;
    GetLocksWithOwnership(OurLocks, OtherLocks);

    // Check if we already have this file locked
    if (OurLocks.Contains(RelativePath) || OurLocks.Contains(File))
    {
        // Already locked by us
        return true;
    }

    // Check if locked by someone else
    const FString *OtherOwner = OtherLocks.Find(RelativePath);
    if (!OtherOwner)
    {
        OtherOwner = OtherLocks.Find(File);
    }
    if (OtherOwner)
    {
        // Locked by someone else
        OutError = FString::Printf(TEXT("File '%s' is locked by %s"), *FPaths::GetCleanFilename(File), **OtherOwner);
        return false;
    }

    // Try to lock the file
    FString Error;
    if (LockFile(File, Error))
    {
        return true;
    }

    // Lock failed - check if it's because someone else just locked it
    OutError = FString::Printf(TEXT("Failed to lock file '%s': %s"), *FPaths::GetCleanFilename(File), *Error);
    return false;
}

bool FCustomGitOperations::ForceLockFile(const FString &File, FString &OutError)
{
    TArray<FString> Results, Errors;
    // git lfs lock --force "file"
    // --force steals the lock from another user
    if (RunGitLFSCommand(TEXT("lock"), {TEXT("--force")}, {File}, Results, Errors))
    {
        InvalidateLockCache(); // Cache is now stale
        return true;
    }

    if (Errors.Num() > 0)
    {
        OutError = Errors[0];
    }
    return false;
}

bool FCustomGitOperations::SetFileReadOnly(const FString &File, bool bReadOnly)
{
    IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (bReadOnly)
    {
        // Make file read-only
        return PlatformFile.SetReadOnly(*File, true);
    }
    else
    {
        // Make file writable
        return PlatformFile.SetReadOnly(*File, false);
    }
}

FString FCustomGitOperations::GetCurrentUserName()
{
    if (CachedUserName.IsEmpty())
    {
        TArray<FString> Results, Errors;
        if (RunGitCommand(TEXT("config"), {TEXT("user.name")}, TArray<FString>(), Results, Errors))
        {
            if (Results.Num() > 0)
            {
                CachedUserName = Results[0].TrimStartAndEnd();
            }
        }
    }
    return CachedUserName;
}

bool FCustomGitOperations::IsLockedByCurrentUser(const FString &File, FString *OutOwner)
{
    TMap<FString, FString> Locks;
    GetAllLocks(Locks);

    // Get the relative path for comparison
    FString RepoRoot = GetRepositoryRoot();
    FString RelativePath = File;
    FPaths::MakePathRelativeTo(RelativePath, *RepoRoot);
    RelativePath = RelativePath.Replace(TEXT("\\"), TEXT("/"));

    // Also try with just the filename for matching
    FString FileName = FPaths::GetCleanFilename(File);

    // Check multiple path formats
    const FString *Owner = Locks.Find(RelativePath);
    if (!Owner)
    {
        Owner = Locks.Find(File);
    }
    if (!Owner)
    {
        // Try to find by filename (less precise but handles path mismatches)
        for (const auto &LockPair : Locks)
        {
            if (LockPair.Key.EndsWith(FileName) || FPaths::GetCleanFilename(LockPair.Key) == FileName)
            {
                Owner = &LockPair.Value;
                break;
            }
        }
    }

    if (Owner)
    {
        if (OutOwner)
        {
            *OutOwner = *Owner;
        }

        // Compare with current user
        FString CurrentUser = GetCurrentUserName();
        if (!CurrentUser.IsEmpty())
        {
            // Case-insensitive comparison (git usernames may vary in case)
            return Owner->Equals(CurrentUser, ESearchCase::IgnoreCase);
        }
    }

    return false;
}
