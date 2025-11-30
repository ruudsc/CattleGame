// Copyright CattleGame. All Rights Reserved.

#include "BlueprintJsonValidator.h"
#include "JsonToBlueprint.h"
#include "Misc/FileHelper.h"
#include "UObject/UObjectIterator.h"

FBlueprintJsonValidationResult FBlueprintJsonValidator::Validate(const FBlueprintJsonData &JsonData)
{
    FBlueprintJsonValidationResult Result;
    Result.bIsValid = true;

    // Validate metadata
    ValidateMetadata(JsonData.Metadata, Result.Issues);

    // Validate variables
    ValidateVariables(JsonData.Variables, Result.Issues);

    // Validate event graphs
    for (const FBlueprintJsonGraph &Graph : JsonData.EventGraphs)
    {
        ValidateGraph(Graph, Result.Issues);
    }

    // Validate functions
    for (const FBlueprintJsonFunction &Function : JsonData.Functions)
    {
        ValidateGraph(Function.Graph, Result.Issues);
    }

    // Update validity based on errors
    Result.bIsValid = !Result.HasErrors();

    return Result;
}

FBlueprintJsonValidationResult FBlueprintJsonValidator::ValidateJsonString(const FString &JsonString)
{
    FBlueprintJsonValidationResult Result;

    FBlueprintJsonData JsonData;
    if (!FJsonToBlueprint::ParseJsonString(JsonString, JsonData))
    {
        Result.bIsValid = false;
        Result.Issues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            TEXT(""),
            TEXT(""),
            TEXT("Failed to parse JSON string")));
        return Result;
    }

    return Validate(JsonData);
}

FBlueprintJsonValidationResult FBlueprintJsonValidator::ValidateJsonFile(const FString &FilePath)
{
    FBlueprintJsonValidationResult Result;

    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        Result.bIsValid = false;
        Result.Issues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            TEXT(""),
            TEXT(""),
            FString::Printf(TEXT("Failed to read file: %s"), *FilePath)));
        return Result;
    }

    return ValidateJsonString(JsonString);
}

void FBlueprintJsonValidator::ValidateNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    // Check required base properties
    if (NodeData.NodeClass.IsEmpty())
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            NodeData.NodeGuid,
            TEXT("NodeClass"),
            TEXT("NodeClass is required")));
        return;
    }

    // Check if node class is valid
    UClass *NodeClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/BlueprintGraph.%s"), *NodeData.NodeClass));
    if (!NodeClass)
    {
        NodeClass = FindFirstObjectSafe<UClass>(*NodeData.NodeClass);
    }

    if (!NodeClass)
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            NodeData.NodeGuid,
            TEXT("NodeClass"),
            FString::Printf(TEXT("Unknown node class: %s"), *NodeData.NodeClass)));
        return;
    }

    // Node-type-specific validation
    if (NodeData.NodeClass == TEXT("K2Node_CallFunction"))
    {
        ValidateCallFunctionNode(NodeData, OutIssues);
    }
    else if (NodeData.NodeClass == TEXT("K2Node_Event") || NodeData.NodeClass == TEXT("K2Node_CustomEvent"))
    {
        ValidateEventNode(NodeData, OutIssues);
    }
    else if (NodeData.NodeClass == TEXT("K2Node_VariableGet") || NodeData.NodeClass == TEXT("K2Node_VariableSet"))
    {
        ValidateVariableNode(NodeData, OutIssues);
    }
    else if (NodeData.NodeClass == TEXT("K2Node_DynamicCast") || NodeData.NodeClass == TEXT("K2Node_ClassDynamicCast"))
    {
        ValidateCastNode(NodeData, OutIssues);
    }
}

bool FBlueprintJsonValidator::CanResolveClass(const FString &ClassPath)
{
    if (ClassPath.IsEmpty())
    {
        return false;
    }

    UClass *FoundClass = FindObject<UClass>(nullptr, *ClassPath);
    if (FoundClass)
    {
        return true;
    }

    FoundClass = LoadObject<UClass>(nullptr, *ClassPath);
    if (FoundClass)
    {
        return true;
    }

    // Try parsing the path
    if (ClassPath.StartsWith(TEXT("/Script/")))
    {
        FString PathWithoutScript = ClassPath.RightChop(8);
        int32 DotIndex;
        if (PathWithoutScript.FindChar(TEXT('.'), DotIndex))
        {
            FString ClassName = PathWithoutScript.RightChop(DotIndex + 1);
            FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
            if (FoundClass)
            {
                return true;
            }
        }
    }

    return false;
}

bool FBlueprintJsonValidator::CanResolveFunctionReference(const FBlueprintJsonMemberReference &MemberRef)
{
    if (MemberRef.MemberName.IsEmpty())
    {
        return false;
    }

    // If we have a parent class, try to find the function there
    if (!MemberRef.MemberParentClass.IsEmpty())
    {
        UClass *ParentClass = nullptr;

        // Try direct lookup
        ParentClass = FindObject<UClass>(nullptr, *MemberRef.MemberParentClass);
        if (!ParentClass)
        {
            ParentClass = LoadObject<UClass>(nullptr, *MemberRef.MemberParentClass);
        }

        if (ParentClass)
        {
            UFunction *Func = ParentClass->FindFunctionByName(*MemberRef.MemberName);
            return Func != nullptr;
        }
    }

    // If self-context, we can't verify without the Blueprint context
    if (MemberRef.bIsSelfContext)
    {
        return true; // Assume valid for self-context
    }

    // Search all loaded classes as fallback (expensive)
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass *Class = *ClassIt;
        UFunction *Func = Class->FindFunctionByName(*MemberRef.MemberName, EIncludeSuperFlag::ExcludeSuper);
        if (Func)
        {
            return true;
        }
    }

    return false;
}

void FBlueprintJsonValidator::ValidateMetadata(const FBlueprintJsonMetadata &Metadata, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    if (Metadata.BlueprintName.IsEmpty())
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Warning,
            TEXT(""),
            TEXT("BlueprintName"),
            TEXT("BlueprintName is empty")));
    }

    if (!Metadata.ParentClass.IsEmpty() && !CanResolveClass(Metadata.ParentClass))
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            TEXT(""),
            TEXT("ParentClass"),
            FString::Printf(TEXT("Cannot resolve parent class: %s"), *Metadata.ParentClass)));
    }

    // Check version compatibility
    if (!Metadata.SerializerVersion.IsEmpty() && Metadata.SerializerVersion != BLUEPRINT_SERIALIZER_VERSION)
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Warning,
            TEXT(""),
            TEXT("SerializerVersion"),
            FString::Printf(TEXT("Version mismatch: JSON is %s, current is %s"), *Metadata.SerializerVersion, BLUEPRINT_SERIALIZER_VERSION)));
    }
}

void FBlueprintJsonValidator::ValidateVariables(const TArray<FBlueprintJsonVariable> &Variables, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    TSet<FString> SeenNames;
    TSet<FString> SeenGuids;

    for (const FBlueprintJsonVariable &Var : Variables)
    {
        if (Var.VarName.IsEmpty())
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Error,
                TEXT(""),
                TEXT("VarName"),
                TEXT("Variable name is empty")));
            continue;
        }

        // Check for duplicates
        if (SeenNames.Contains(Var.VarName))
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Error,
                TEXT(""),
                TEXT("VarName"),
                FString::Printf(TEXT("Duplicate variable name: %s"), *Var.VarName)));
        }
        SeenNames.Add(Var.VarName);

        if (!Var.VarGuid.IsEmpty())
        {
            if (SeenGuids.Contains(Var.VarGuid))
            {
                OutIssues.Add(FBlueprintJsonValidationIssue(
                    FBlueprintJsonValidationIssue::ESeverity::Error,
                    TEXT(""),
                    TEXT("VarGuid"),
                    FString::Printf(TEXT("Duplicate variable GUID: %s"), *Var.VarGuid)));
            }
            SeenGuids.Add(Var.VarGuid);
        }

        // Validate type
        if (Var.VarType.IsEmpty())
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Error,
                TEXT(""),
                TEXT("VarType"),
                FString::Printf(TEXT("Variable '%s' has no type"), *Var.VarName)));
        }
    }
}

void FBlueprintJsonValidator::ValidateGraph(const FBlueprintJsonGraph &Graph, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    TSet<FString> SeenNodeGuids;

    for (const FBlueprintJsonNode &Node : Graph.Nodes)
    {
        // Check for duplicate GUIDs
        if (!Node.NodeGuid.IsEmpty())
        {
            if (SeenNodeGuids.Contains(Node.NodeGuid))
            {
                OutIssues.Add(FBlueprintJsonValidationIssue(
                    FBlueprintJsonValidationIssue::ESeverity::Error,
                    Node.NodeGuid,
                    TEXT("NodeGuid"),
                    TEXT("Duplicate node GUID in graph")));
            }
            SeenNodeGuids.Add(Node.NodeGuid);
        }

        // Validate the node
        ValidateNode(Node, OutIssues);
    }

    // Validate pin connections
    ValidatePinConnections(Graph, OutIssues);
}

void FBlueprintJsonValidator::ValidatePinConnections(const FBlueprintJsonGraph &Graph, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    // Build a set of all pin IDs in the graph
    TSet<FString> AllPinIds;
    for (const FBlueprintJsonNode &Node : Graph.Nodes)
    {
        for (const FBlueprintJsonPin &Pin : Node.Pins)
        {
            if (!Pin.PinId.IsEmpty())
            {
                AllPinIds.Add(Pin.PinId);
            }
        }
    }

    // Check that all connections reference valid pins
    for (const FBlueprintJsonNode &Node : Graph.Nodes)
    {
        for (const FBlueprintJsonPin &Pin : Node.Pins)
        {
            for (const FString &LinkedPinId : Pin.LinkedTo)
            {
                if (!AllPinIds.Contains(LinkedPinId))
                {
                    OutIssues.Add(FBlueprintJsonValidationIssue(
                        FBlueprintJsonValidationIssue::ESeverity::Warning,
                        Node.NodeGuid,
                        Pin.PinName,
                        FString::Printf(TEXT("Pin connection references unknown pin: %s"), *LinkedPinId)));
                }
            }
        }
    }
}

void FBlueprintJsonValidator::ValidateCallFunctionNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    // Check if function reference is set
    if (NodeData.FunctionReference.MemberName.IsEmpty())
    {
        // Check legacy format
        const FString *LegacyRef = NodeData.NodeSpecificData.Find(TEXT("FunctionReference"));
        if (!LegacyRef || LegacyRef->IsEmpty())
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Error,
                NodeData.NodeGuid,
                TEXT("FunctionReference"),
                TEXT("CallFunction node has no function reference")));
            return;
        }
    }

    // Try to resolve the function
    if (!NodeData.FunctionReference.MemberName.IsEmpty() &&
        !CanResolveFunctionReference(NodeData.FunctionReference))
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Warning,
            NodeData.NodeGuid,
            TEXT("FunctionReference"),
            FString::Printf(TEXT("Cannot resolve function: %s in %s"),
                            *NodeData.FunctionReference.MemberName,
                            *NodeData.FunctionReference.MemberParentClass)));
    }
}

void FBlueprintJsonValidator::ValidateEventNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    if (NodeData.NodeClass == TEXT("K2Node_CustomEvent"))
    {
        if (NodeData.CustomEventName.IsEmpty())
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Error,
                NodeData.NodeGuid,
                TEXT("CustomEventName"),
                TEXT("CustomEvent node has no event name")));
        }
    }
    else
    {
        // Regular event - check EventReference
        if (NodeData.EventReference.MemberName.IsEmpty())
        {
            OutIssues.Add(FBlueprintJsonValidationIssue(
                FBlueprintJsonValidationIssue::ESeverity::Warning,
                NodeData.NodeGuid,
                TEXT("EventReference"),
                TEXT("Event node has no event reference")));
        }
    }
}

void FBlueprintJsonValidator::ValidateVariableNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    if (NodeData.VariableReference.MemberName.IsEmpty())
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            NodeData.NodeGuid,
            TEXT("VariableReference"),
            FString::Printf(TEXT("%s node has no variable reference"), *NodeData.NodeClass)));
    }
}

void FBlueprintJsonValidator::ValidateCastNode(const FBlueprintJsonNode &NodeData, TArray<FBlueprintJsonValidationIssue> &OutIssues)
{
    if (NodeData.TargetClass.IsEmpty())
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            NodeData.NodeGuid,
            TEXT("TargetClass"),
            TEXT("Cast node has no target class")));
        return;
    }

    if (!CanResolveClass(NodeData.TargetClass))
    {
        OutIssues.Add(FBlueprintJsonValidationIssue(
            FBlueprintJsonValidationIssue::ESeverity::Error,
            NodeData.NodeGuid,
            TEXT("TargetClass"),
            FString::Printf(TEXT("Cannot resolve target class: %s"), *NodeData.TargetClass)));
    }
}
