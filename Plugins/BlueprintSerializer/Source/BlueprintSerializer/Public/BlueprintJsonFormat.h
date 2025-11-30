// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintJsonFormat.generated.h"

/** Schema version for JSON format compatibility */
#define BLUEPRINT_SERIALIZER_VERSION TEXT("2.0.0")

// ============================================================================
// Member Reference Types (for functions, events, variables, delegates)
// ============================================================================

/** Serialized member reference - used for functions, events, variables, delegates */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonMemberReference
{
    GENERATED_BODY()

    /** Name of the member (function name, variable name, etc.) */
    UPROPERTY()
    FString MemberName;

    /** Class that owns this member (e.g., "/Script/Engine.KismetSystemLibrary") */
    UPROPERTY()
    FString MemberParentClass;

    /** GUID of the member (for Blueprint-defined members) */
    UPROPERTY()
    FString MemberGuid;

    /** Whether this is a self-context reference */
    UPROPERTY()
    bool bIsSelfContext = false;

    /** Whether the function is const */
    UPROPERTY()
    bool bIsConstFunc = false;

    /** Whether the member is local to the graph/function */
    UPROPERTY()
    bool bIsLocalScope = false;
};

// ============================================================================
// Node Schema Types (for validation and documentation)
// ============================================================================

/** Property requirement level */
UENUM()
enum class EBlueprintSchemaRequirement : uint8
{
    Required, // Must be present and valid
    Optional, // Can be omitted
    Computed  // Generated during deserialization, not stored
};

/** Schema definition for a single property on a node type */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintNodeSchemaProperty
{
    GENERATED_BODY()

    /** Property name in JSON */
    UPROPERTY()
    FString PropertyName;

    /** JSON type: "string", "number", "boolean", "object", "array", "MemberReference" */
    UPROPERTY()
    FString PropertyType;

    /** Whether this property is required */
    UPROPERTY()
    EBlueprintSchemaRequirement Requirement = EBlueprintSchemaRequirement::Optional;

    /** Human-readable description */
    UPROPERTY()
    FString Description;

    /** Default value if not specified */
    UPROPERTY()
    FString DefaultValue;
};

/** Schema definition for a K2Node type */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintNodeSchema
{
    GENERATED_BODY()

    /** Node class name (e.g., "K2Node_CallFunction") */
    UPROPERTY()
    FString NodeClass;

    /** Human-readable display name */
    UPROPERTY()
    FString DisplayName;

    /** Category for grouping (e.g., "Flow Control", "Function Calls") */
    UPROPERTY()
    FString Category;

    /** Description of what this node does */
    UPROPERTY()
    FString Description;

    /** Parent node class (for inheritance) */
    UPROPERTY()
    FString ParentNodeClass;

    /** Properties specific to this node type */
    UPROPERTY()
    TArray<FBlueprintNodeSchemaProperty> Properties;

    /** Whether this node can have dynamic pins */
    UPROPERTY()
    bool bHasDynamicPins = false;

    /** Whether this node is latent (has execution flow) */
    UPROPERTY()
    bool bIsLatent = false;

    /** Whether this node can be pure (no exec pins) */
    UPROPERTY()
    bool bCanBePure = false;
};

/** Complete schema for all known node types */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintSerializerSchema
{
    GENERATED_BODY()

    /** Engine version this schema was generated for */
    UPROPERTY()
    FString EngineVersion;

    /** Schema version */
    UPROPERTY()
    FString SchemaVersion;

    /** Timestamp when schema was generated */
    UPROPERTY()
    FString GeneratedTimestamp;

    /** All known node type schemas */
    UPROPERTY()
    TArray<FBlueprintNodeSchema> NodeSchemas;
};

// ============================================================================
// Pin and Node Types
// ============================================================================

/** Pin data in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonPin
{
    GENERATED_BODY()

    UPROPERTY()
    FString PinId;

    UPROPERTY()
    FString PinName;

    UPROPERTY()
    FString Direction; // "input" or "output"

    UPROPERTY()
    FString PinType;

    UPROPERTY()
    FString DefaultValue;

    UPROPERTY()
    TArray<FString> LinkedTo; // Array of connected pin IDs
};

/** Node data in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonNode
{
    GENERATED_BODY()

    UPROPERTY()
    FString NodeGuid;

    UPROPERTY()
    FString NodeClass;

    UPROPERTY()
    FString NodeTitle;

    UPROPERTY()
    FString NodeComment;

    UPROPERTY()
    float PositionX = 0.f;

    UPROPERTY()
    float PositionY = 0.f;

    UPROPERTY()
    TArray<FBlueprintJsonPin> Pins;

    // ===== Node-Type-Specific References =====

    /** For K2Node_CallFunction, K2Node_CallParentFunction, etc. */
    UPROPERTY()
    FBlueprintJsonMemberReference FunctionReference;

    /** For K2Node_Event, K2Node_CustomEvent, etc. */
    UPROPERTY()
    FBlueprintJsonMemberReference EventReference;

    /** For K2Node_VariableGet, K2Node_VariableSet, etc. */
    UPROPERTY()
    FBlueprintJsonMemberReference VariableReference;

    /** For K2Node_AddDelegate, K2Node_CallDelegate, etc. */
    UPROPERTY()
    FBlueprintJsonMemberReference DelegateReference;

    // ===== Type References =====

    /** For K2Node_DynamicCast, K2Node_ClassDynamicCast - target class path */
    UPROPERTY()
    FString TargetClass;

    /** For K2Node_SpawnActorFromClass, K2Node_ConstructObjectFromClass - class to spawn */
    UPROPERTY()
    FString SpawnClass;

    /** For K2Node_SwitchEnum, K2Node_CastByteToEnum - enum type */
    UPROPERTY()
    FString EnumType;

    /** For K2Node_MakeStruct, K2Node_BreakStruct - struct type */
    UPROPERTY()
    FString StructType;

    // ===== Special Node Data =====

    /** For K2Node_MacroInstance - macro graph reference */
    UPROPERTY()
    FString MacroReference;

    /** For K2Node_Timeline - timeline name */
    UPROPERTY()
    FString TimelineName;

    /** For K2Node_Literal - literal value */
    UPROPERTY()
    FString LiteralValue;

    /** For K2Node_CustomEvent - custom event name */
    UPROPERTY()
    FString CustomEventName;

    /** For K2Node_InputAction - action name */
    UPROPERTY()
    FString InputActionName;

    /** For K2Node_InputKey - key */
    UPROPERTY()
    FString InputKey;

    // ===== Flags =====

    /** Whether node is pure (no exec pins) */
    UPROPERTY()
    bool bIsPure = false;

    /** Whether node is latent */
    UPROPERTY()
    bool bIsLatent = false;

    /** For variable nodes - whether to use local copy */
    UPROPERTY()
    bool bSelfContext = false;

    // ===== Legacy/Fallback =====

    /** Additional node-specific data not covered above (for extensibility) */
    UPROPERTY()
    TMap<FString, FString> NodeSpecificData;
};

/** Graph data in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonGraph
{
    GENERATED_BODY()

    UPROPERTY()
    FString GraphName;

    UPROPERTY()
    FString GraphType; // "EventGraph", "Function", "Macro"

    UPROPERTY()
    FString GraphGuid;

    UPROPERTY()
    TArray<FBlueprintJsonNode> Nodes;
};

/** Variable data in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonVariable
{
    GENERATED_BODY()

    UPROPERTY()
    FString VarName;

    UPROPERTY()
    FString VarGuid;

    UPROPERTY()
    FString VarType;

    UPROPERTY()
    FString Category;

    UPROPERTY()
    FString DefaultValue;

    UPROPERTY()
    bool bIsExposed = false;

    UPROPERTY()
    bool bIsReadOnly = false;

    UPROPERTY()
    FString ReplicationCondition;

    UPROPERTY()
    TMap<FString, FString> Metadata;
};

/** Component data in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonComponent
{
    GENERATED_BODY()

    UPROPERTY()
    FString ComponentName;

    UPROPERTY()
    FString ComponentClass;

    UPROPERTY()
    FString ParentComponent;

    UPROPERTY()
    TMap<FString, FString> Properties;
};

/** Function signature in serialized format */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonFunction
{
    GENERATED_BODY()

    UPROPERTY()
    FString FunctionName;

    UPROPERTY()
    FString FunctionGuid;

    UPROPERTY()
    TArray<FBlueprintJsonVariable> Parameters;

    UPROPERTY()
    TArray<FBlueprintJsonVariable> ReturnValues;

    UPROPERTY()
    FBlueprintJsonGraph Graph;

    UPROPERTY()
    bool bIsStatic = false;

    UPROPERTY()
    bool bIsPure = false;

    UPROPERTY()
    bool bIsConst = false;
};

/** Metadata for the serialized blueprint */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonMetadata
{
    GENERATED_BODY()

    UPROPERTY()
    FString BlueprintName;

    UPROPERTY()
    FString BlueprintPath;

    UPROPERTY()
    FString BlueprintType; // "Normal", "Const", "MacroLibrary", "Interface", "LevelScript", "FunctionLibrary"

    UPROPERTY()
    FString ParentClass;

    UPROPERTY()
    FString GeneratedClass;

    UPROPERTY()
    FString EngineVersion;

    UPROPERTY()
    FString SerializerVersion;

    UPROPERTY()
    FString ExportTimestamp;
};

/** Root structure for serialized blueprint */
USTRUCT()
struct BLUEPRINTSERIALIZER_API FBlueprintJsonData
{
    GENERATED_BODY()

    UPROPERTY()
    FBlueprintJsonMetadata Metadata;

    UPROPERTY()
    TArray<FBlueprintJsonVariable> Variables;

    UPROPERTY()
    TArray<FBlueprintJsonGraph> EventGraphs;

    UPROPERTY()
    TArray<FBlueprintJsonFunction> Functions;

    UPROPERTY()
    TArray<FBlueprintJsonGraph> Macros;

    UPROPERTY()
    TArray<FBlueprintJsonComponent> Components;

    UPROPERTY()
    TArray<FString> ImplementedInterfaces;

    UPROPERTY()
    TArray<FString> HardDependencies;

    UPROPERTY()
    TArray<FString> SoftDependencies;
};
