// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
struct FBlueprintJsonData;
struct FBlueprintJsonMemberReference;
struct FMemberReference;

/**
 * Utility class for deserializing JSON to Blueprint assets
 */
class BLUEPRINTSERIALIZEREDITOR_API FJsonToBlueprint
{
public:
    /**
     * Deserialize JSON data to create a new Blueprint asset
     * @param JsonData The JSON data structure to deserialize
     * @param PackagePath The package path where the Blueprint will be created
     * @param BlueprintName The name for the new Blueprint
     * @return The created Blueprint, or nullptr if deserialization failed
     */
    static UBlueprint *DeserializeBlueprint(const FBlueprintJsonData &JsonData, const FString &PackagePath, const FString &BlueprintName);

    /**
     * Deserialize JSON string to create a new Blueprint asset
     * @param JsonString The JSON string to deserialize
     * @param PackagePath The package path where the Blueprint will be created
     * @param BlueprintName The name for the new Blueprint
     * @return The created Blueprint, or nullptr if deserialization failed
     */
    static UBlueprint *DeserializeBlueprintFromString(const FString &JsonString, const FString &PackagePath, const FString &BlueprintName);

    /**
     * Deserialize JSON file to create a new Blueprint asset
     * @param FilePath The path to the JSON file
     * @param PackagePath The package path where the Blueprint will be created
     * @param BlueprintName The name for the new Blueprint (if empty, uses name from JSON)
     * @return The created Blueprint, or nullptr if deserialization failed
     */
    static UBlueprint *DeserializeBlueprintFromFile(const FString &FilePath, const FString &PackagePath, const FString &BlueprintName = TEXT(""));

    /**
     * Merge JSON data into an existing Blueprint
     * This adds new nodes/variables from JSON without removing existing ones
     * @param TargetBlueprint The existing Blueprint to merge into
     * @param JsonData The JSON data to merge
     * @return True if merge succeeded
     */
    static bool MergeJsonIntoBlueprint(UBlueprint *TargetBlueprint, const FBlueprintJsonData &JsonData);

    /**
     * Merge JSON file into an existing Blueprint
     * @param TargetBlueprint The existing Blueprint to merge into
     * @param FilePath The path to the JSON file
     * @return True if merge succeeded
     */
    static bool MergeJsonFileIntoBlueprint(UBlueprint *TargetBlueprint, const FString &FilePath);

    /**
     * Parse JSON string to FBlueprintJsonData structure
     * @param JsonString The JSON string to parse
     * @param OutJsonData The output data structure
     * @return True if parsing succeeded
     */
    static bool ParseJsonString(const FString &JsonString, FBlueprintJsonData &OutJsonData);

private:
    /** Find the parent class from the JSON metadata */
    static UClass *FindParentClass(const FBlueprintJsonData &JsonData);

    /** Create the base Blueprint asset */
    static UBlueprint *CreateBlueprintAsset(const FBlueprintJsonData &JsonData, const FString &PackagePath, const FString &BlueprintName, UClass *ParentClass);

    /** Deserialize variables into the Blueprint */
    static void DeserializeVariables(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Merge variables from JSON into Blueprint (adds new, doesn't remove existing) */
    static void MergeVariables(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Deserialize event graphs into the Blueprint */
    static void DeserializeEventGraphs(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Merge event graph nodes from JSON into Blueprint (adds new nodes) */
    static void MergeEventGraphs(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Deserialize functions into the Blueprint */
    static void DeserializeFunctions(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Deserialize components into the Blueprint */
    static void DeserializeComponents(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Deserialize interfaces into the Blueprint */
    static void DeserializeInterfaces(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData);

    /** Helper to deserialize a graph */
    static void DeserializeGraph(class UEdGraph *Graph, const struct FBlueprintJsonGraph &GraphData);

    /** Helper to create and deserialize a node */
    static class UEdGraphNode *DeserializeNode(class UEdGraph *Graph, const struct FBlueprintJsonNode &NodeData);

    /** Helper to link pins after all nodes are created */
    static void LinkPins(class UEdGraph *Graph, const struct FBlueprintJsonGraph &GraphData);

    /** Convert string representation to pin type */
    static bool StringToPinType(const FString &TypeString, struct FEdGraphPinType &OutPinType);

    /** Compile the Blueprint after deserialization */
    static bool CompileBlueprint(UBlueprint *Blueprint);

    /** Resolve a FBlueprintJsonMemberReference to a UFunction* */
    static UFunction *ResolveMemberReferenceAsFunction(const FBlueprintJsonMemberReference &MemberRef);

    /** Find a class from a path string */
    static UClass *FindParentClassFromPath(const FString &ClassPath);

    /** Resolve a function from a full path string (legacy format) */
    static UFunction *ResolveFunctionFromPath(const FString &FunctionPath);

    /** Apply a JSON member reference to a UE FMemberReference (for variables) */
    static void ApplyMemberReferenceToVariable(const FBlueprintJsonMemberReference &MemberRef, FMemberReference &OutMemberRef, class UEdGraph *Graph);
};
