// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "BlueprintSerializerSubsystem.generated.h"

class UBlueprint;

/**
 * Editor subsystem for Blueprint serialization operations.
 * Exposes Blueprint import/export/merge functionality to Python scripting.
 */
UCLASS()
class BLUEPRINTSERIALIZEREDITOR_API UBlueprintSerializerSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    /**
     * Serialize a Blueprint to JSON string
     * @param Blueprint The Blueprint to serialize
     * @return JSON string representation
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    FString SerializeBlueprintToString(UBlueprint *Blueprint);

    /**
     * Deserialize a Blueprint from JSON string
     * @param JsonString The JSON string to deserialize
     * @param PackagePath The package path for the new Blueprint
     * @param BlueprintName The name for the new Blueprint (empty = use name from JSON)
     * @return The created Blueprint, or nullptr on failure
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    UBlueprint *DeserializeBlueprintFromString(const FString &JsonString, const FString &PackagePath, const FString &BlueprintName);

    /**
     * Merge JSON string into an existing Blueprint (adds new nodes without removing existing ones)
     * @param TargetBlueprint The Blueprint to merge into
     * @param JsonString The JSON string with new content
     * @return True if merge succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    bool MergeJsonStringIntoBlueprint(UBlueprint *TargetBlueprint, const FString &JsonString);

    /**
     * Merge JSON file into an existing Blueprint
     * @param TargetBlueprint The Blueprint to merge into
     * @param FilePath Path to the JSON file
     * @return True if merge succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    bool MergeJsonFileIntoBlueprint(UBlueprint *TargetBlueprint, const FString &FilePath);

    /**
     * Open the merge editor for two Blueprints
     * @param OriginalBlueprint The original Blueprint
     * @param ModifiedBlueprint The modified Blueprint
     * @return True if merge editor was opened
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    bool OpenMergeEditor(UBlueprint *OriginalBlueprint, UBlueprint *ModifiedBlueprint);

    /**
     * Get a diff summary between two Blueprints
     * @param BlueprintA First Blueprint
     * @param BlueprintB Second Blueprint
     * @return Human-readable diff summary
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer")
    FString GetDiffSummary(UBlueprint *BlueprintA, UBlueprint *BlueprintB);
};
