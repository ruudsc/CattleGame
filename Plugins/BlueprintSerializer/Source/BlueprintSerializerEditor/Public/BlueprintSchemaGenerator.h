// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintJsonFormat.h"

/**
 * Generates JSON schema definitions for all K2Node types using UE5 reflection.
 * This allows validation of Blueprint JSON and provides documentation for AI agents.
 */
class BLUEPRINTSERIALIZEREDITOR_API FBlueprintSchemaGenerator
{
public:
    /**
     * Generate schema for all known K2Node types.
     * Uses reflection to enumerate all UK2Node subclasses and extract their properties.
     */
    static FBlueprintSerializerSchema GenerateFullSchema();

    /**
     * Generate schema for a specific node class.
     * @param NodeClass The UK2Node subclass to generate schema for.
     * @return Schema definition for the node type.
     */
    static FBlueprintNodeSchema GenerateNodeSchema(UClass *NodeClass);

    /**
     * Export schema to JSON file.
     * @param Schema The schema to export.
     * @param FilePath Output file path.
     * @return True if export succeeded.
     */
    static bool ExportSchemaToFile(const FBlueprintSerializerSchema &Schema, const FString &FilePath);

    /**
     * Export schema to JSON string.
     * @param Schema The schema to export.
     * @param OutJsonString Output JSON string.
     * @return True if export succeeded.
     */
    static bool ExportSchemaToString(const FBlueprintSerializerSchema &Schema, FString &OutJsonString);

    /**
     * Get the cached schema (generates if not yet cached).
     * Schema is cached in memory and optionally to disk.
     */
    static const FBlueprintSerializerSchema &GetCachedSchema();

    /**
     * Force regeneration of the cached schema.
     */
    static void InvalidateCache();

    /**
     * Get the default schema cache file path.
     */
    static FString GetDefaultSchemaFilePath();

    /**
     * Load schema from cache file if it exists and is valid.
     * @param OutSchema Output schema.
     * @return True if loaded from cache, false if regeneration needed.
     */
    static bool LoadSchemaFromCache(FBlueprintSerializerSchema &OutSchema);

    /**
     * Save schema to cache file.
     */
    static bool SaveSchemaToCache(const FBlueprintSerializerSchema &Schema);

    /**
     * Get all K2Node subclasses in the engine.
     * @return Array of UK2Node subclasses.
     */
    static TArray<UClass *> GetAllK2NodeClasses();

    /**
     * Get the category for a node class (public wrapper).
     * @param NodeClass The UK2Node subclass.
     * @return Category string (e.g., "Flow Control", "Math", "Functions").
     */
    static FString GetNodeCategory(UClass *NodeClass);

    /**
     * Check if a node type is latent (has async execution).
     * @param NodeClass The UK2Node subclass.
     * @return True if the node is latent.
     */
    static bool IsNodeLatent(UClass *NodeClass);

private:
    /** Cached schema instance */
    static TOptional<FBlueprintSerializerSchema> CachedSchema;

    /** Determine if a node class can be pure */
    static bool CanNodeBePure(UClass *NodeClass);

    /** Extract properties specific to a node type */
    static void ExtractNodeSpecificProperties(UClass *NodeClass, FBlueprintNodeSchema &OutSchema);

    /** Get description for a node class */
    static FString GetNodeDescription(UClass *NodeClass);

    /** Get display name for a node class */
    static FString GetNodeDisplayName(UClass *NodeClass);
};
