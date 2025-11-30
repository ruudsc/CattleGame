// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
struct FBlueprintJsonData;
struct FBlueprintJsonMemberReference;
struct FMemberReference;

/**
 * Utility class for serializing Blueprint assets to JSON format
 */
class BLUEPRINTSERIALIZEREDITOR_API FBlueprintToJson
{
public:
    /**
     * Serialize a Blueprint asset to JSON data structure
     * @param Blueprint The Blueprint to serialize
     * @param OutJsonData The output JSON data structure
     * @return True if serialization succeeded
     */
    static bool SerializeBlueprint(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /**
     * Serialize a Blueprint asset to JSON string
     * @param Blueprint The Blueprint to serialize
     * @param OutJsonString The output JSON string
     * @param bPrettyPrint Whether to format the JSON with indentation
     * @return True if serialization succeeded
     */
    static bool SerializeBlueprintToString(const UBlueprint *Blueprint, FString &OutJsonString, bool bPrettyPrint = true);

    /**
     * Serialize a Blueprint asset to a JSON file
     * @param Blueprint The Blueprint to serialize
     * @param FilePath The output file path
     * @param bPrettyPrint Whether to format the JSON with indentation
     * @return True if serialization and file write succeeded
     */
    static bool SerializeBlueprintToFile(const UBlueprint *Blueprint, const FString &FilePath, bool bPrettyPrint = true);

private:
    /** Serialize blueprint metadata */
    static void SerializeMetadata(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize blueprint variables */
    static void SerializeVariables(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize blueprint event graphs */
    static void SerializeEventGraphs(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize blueprint functions */
    static void SerializeFunctions(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize blueprint macros */
    static void SerializeMacros(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize blueprint components */
    static void SerializeComponents(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize implemented interfaces */
    static void SerializeInterfaces(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Serialize dependencies */
    static void SerializeDependencies(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData);

    /** Helper to serialize a single graph */
    static void SerializeGraph(const class UEdGraph *Graph, struct FBlueprintJsonGraph &OutGraphData);

    /** Helper to serialize a single node */
    static void SerializeNode(const class UEdGraphNode *Node, struct FBlueprintJsonNode &OutNodeData);

    /** Helper to serialize a pin */
    static void SerializePin(const class UEdGraphPin *Pin, struct FBlueprintJsonPin &OutPinData);

    /** Convert pin type to string representation */
    static FString PinTypeToString(const struct FEdGraphPinType &PinType);

    /** Serialize a member reference (function, event, variable, delegate) */
    static void SerializeMemberReference(const FMemberReference &MemberRef, FBlueprintJsonMemberReference &OutJsonRef);
};
