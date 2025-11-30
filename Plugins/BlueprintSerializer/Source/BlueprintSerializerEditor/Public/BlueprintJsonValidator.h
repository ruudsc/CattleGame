// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintJsonFormat.h"

/**
 * Validation result for a single issue
 */
struct BLUEPRINTSERIALIZEREDITOR_API FBlueprintJsonValidationIssue
{
    enum class ESeverity : uint8
    {
        Error,   // Will cause deserialization to fail
        Warning, // May cause problems, but won't prevent deserialization
        Info     // Informational message about potential issues
    };

    ESeverity Severity = ESeverity::Error;
    FString NodeGuid;
    FString PropertyName;
    FString Message;

    FBlueprintJsonValidationIssue() = default;
    FBlueprintJsonValidationIssue(ESeverity InSeverity, const FString &InNodeGuid, const FString &InPropertyName, const FString &InMessage)
        : Severity(InSeverity), NodeGuid(InNodeGuid), PropertyName(InPropertyName), Message(InMessage) {}
};

/**
 * Validation result for an entire Blueprint JSON
 */
struct BLUEPRINTSERIALIZEREDITOR_API FBlueprintJsonValidationResult
{
    bool bIsValid = true;
    TArray<FBlueprintJsonValidationIssue> Issues;

    bool HasErrors() const
    {
        for (const FBlueprintJsonValidationIssue &Issue : Issues)
        {
            if (Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Error)
            {
                return true;
            }
        }
        return false;
    }

    bool HasWarnings() const
    {
        for (const FBlueprintJsonValidationIssue &Issue : Issues)
        {
            if (Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Warning)
            {
                return true;
            }
        }
        return false;
    }

    FString ToString() const
    {
        FString Result;
        for (const FBlueprintJsonValidationIssue &Issue : Issues)
        {
            FString SeverityStr;
            switch (Issue.Severity)
            {
            case FBlueprintJsonValidationIssue::ESeverity::Error:
                SeverityStr = TEXT("ERROR");
                break;
            case FBlueprintJsonValidationIssue::ESeverity::Warning:
                SeverityStr = TEXT("WARNING");
                break;
            case FBlueprintJsonValidationIssue::ESeverity::Info:
                SeverityStr = TEXT("INFO");
                break;
            }

            if (!Issue.NodeGuid.IsEmpty())
            {
                Result += FString::Printf(TEXT("[%s] Node %s: %s\n"), *SeverityStr, *Issue.NodeGuid, *Issue.Message);
            }
            else
            {
                Result += FString::Printf(TEXT("[%s] %s\n"), *SeverityStr, *Issue.Message);
            }
        }
        return Result;
    }
};

/**
 * Validates Blueprint JSON data against schema and semantic rules.
 */
class BLUEPRINTSERIALIZEREDITOR_API FBlueprintJsonValidator
{
public:
    /**
     * Validate Blueprint JSON data.
     * @param JsonData The data to validate.
     * @return Validation result with any issues found.
     */
    static FBlueprintJsonValidationResult Validate(const FBlueprintJsonData &JsonData);

    /**
     * Validate a JSON string.
     * @param JsonString The JSON string to validate.
     * @return Validation result with any issues found.
     */
    static FBlueprintJsonValidationResult ValidateJsonString(const FString &JsonString);

    /**
     * Validate a JSON file.
     * @param FilePath Path to the JSON file.
     * @return Validation result with any issues found.
     */
    static FBlueprintJsonValidationResult ValidateJsonFile(const FString &FilePath);

    /**
     * Validate a single node against schema.
     * @param NodeData The node to validate.
     * @param OutIssues Array to add validation issues to.
     */
    static void ValidateNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /**
     * Check if a class path is resolvable.
     * @param ClassPath The class path to check.
     * @return True if the class can be found.
     */
    static bool CanResolveClass(const FString &ClassPath);

    /**
     * Check if a function reference is resolvable.
     * @param MemberRef The function reference to check.
     * @return True if the function can be found.
     */
    static bool CanResolveFunctionReference(const FBlueprintJsonMemberReference &MemberRef);

private:
    /** Validate metadata */
    static void ValidateMetadata(const FBlueprintJsonMetadata &Metadata, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate variables */
    static void ValidateVariables(const TArray<FBlueprintJsonVariable> &Variables, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate a graph */
    static void ValidateGraph(const FBlueprintJsonGraph &Graph, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate pin connections */
    static void ValidatePinConnections(const FBlueprintJsonGraph &Graph, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate K2Node_CallFunction node */
    static void ValidateCallFunctionNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate K2Node_Event node */
    static void ValidateEventNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate K2Node_VariableGet/Set node */
    static void ValidateVariableNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues);

    /** Validate K2Node_DynamicCast node */
    static void ValidateCastNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues);
};
