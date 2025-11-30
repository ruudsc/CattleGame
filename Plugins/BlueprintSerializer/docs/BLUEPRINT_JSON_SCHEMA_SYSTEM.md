# Blueprint JSON Schema System

Complete implementation for comprehensive Blueprint JSON serialization with schema validation and editor tooling.

## Overview

The Blueprint JSON Schema System provides:
- **Reflection-based schema generation** for all 120+ K2Node types
- **Rich serialization** with node-type-specific fields (FunctionReference, EventReference, etc.)
- **Robust deserialization** with proper FMemberReference API usage
- **Validation tools** for verifying Blueprint JSON before import
- **Editor integration** via context menus and console commands
- **Python utilities** for programmatic JSON manipulation

---

## Architecture

### Core Components

| Component | File | Purpose |
|-----------|------|---------|
| Format Types | `BlueprintJsonFormat.h` | Data structures for JSON serialization |
| Schema Generator | `BlueprintSchemaGenerator.h/cpp` | Reflection-based schema generation |
| Serializer | `BlueprintToJson.h/cpp` | Blueprint → JSON conversion |
| Deserializer | `JsonToBlueprint.h/cpp` | JSON → Blueprint conversion |
| Validator | `BlueprintJsonValidator.h/cpp` | JSON validation against schema |
| Python Module | `Scripts/blueprint_schema.py` | Python utilities for JSON editing |
| Editor Module | `BlueprintSerializerEditorModule.cpp` | Menu and console command integration |

---

## Step 1: FMemberReference and Schema Types

**File:** `Source/BlueprintSerializer/Public/BlueprintJsonFormat.h`

### FBlueprintJsonMemberReference

Serializes Unreal's `FMemberReference` for function/variable references:

```cpp
struct FBlueprintJsonMemberReference
{
    FString MemberName;           // Function or variable name
    FString MemberParentClass;    // Owning class path (e.g., "/Script/Engine.Actor")
    FString MemberGuid;           // GUID for local members
    bool bSelfContext = false;    // True if referencing self (this)
};
```

### Node-Type-Specific Fields

Added to `FBlueprintJsonNode`:

| Field | Type | Used By |
|-------|------|---------|
| `FunctionReference` | `FBlueprintJsonMemberReference` | K2Node_CallFunction |
| `EventReference` | `FBlueprintJsonMemberReference` | K2Node_Event |
| `VariableReference` | `FBlueprintJsonMemberReference` | K2Node_VariableGet/Set |
| `DelegateReference` | `FBlueprintJsonMemberReference` | K2Node_CreateDelegate |
| `TargetClass` | `FString` | K2Node_DynamicCast |
| `SpawnClass` | `FString` | K2Node_SpawnActorFromClass |
| `MacroReference` | `FString` | K2Node_MacroInstance |
| `TimelineName` | `FString` | K2Node_Timeline |
| `CustomEventName` | `FString` | K2Node_CustomEvent |
| `InputActionName` | `FString` | K2Node_InputAction |
| `InputKey` | `FString` | K2Node_InputKey |
| `bIsPure` | `bool` | Pure function nodes |
| `bIsLatent` | `bool` | Async/latent nodes |

### Schema Version

```cpp
// Schema version bumped to 2.0.0 for new format
FString SchemaVersion = TEXT("2.0.0");
```

---

## Step 2: BlueprintSchemaGenerator

**Files:** 
- `Source/BlueprintSerializerEditor/Public/BlueprintSchemaGenerator.h`
- `Source/BlueprintSerializerEditor/Private/BlueprintSchemaGenerator.cpp`

### Public API

```cpp
class FBlueprintSchemaGenerator
{
public:
    // Generate complete schema for all node types
    static FBlueprintSerializerSchema GenerateFullSchema();
    
    // Generate schema for specific node class
    static FBlueprintNodeSchema GenerateNodeSchema(UClass* NodeClass);
    
    // Get list of all K2Node types (120+)
    static TArray<UClass*> GetAllK2NodeClasses();
    
    // Get category string for a node
    static FString GetNodeCategory(UClass* NodeClass);
    
    // Check if node is latent (async)
    static bool IsNodeLatent(UClass* NodeClass);
    
    // Export to file/string
    static bool ExportSchemaToFile(const FBlueprintSerializerSchema& Schema, const FString& FilePath);
    static bool ExportSchemaToString(const FBlueprintSerializerSchema& Schema, FString& OutJsonString);
    
    // Caching
    static const FBlueprintSerializerSchema& GetCachedSchema();
    static void InvalidateCache();
};
```

### Node Categories

The generator automatically categorizes nodes:

- **Function Calls** - UK2Node_CallFunction and derivatives
- **Events** - UK2Node_Event, UK2Node_CustomEvent
- **Variables** - UK2Node_VariableGet, UK2Node_VariableSet
- **Flow Control** - UK2Node_IfThenElse, UK2Node_Switch*, UK2Node_Select
- **Casting** - UK2Node_DynamicCast
- **Containers** - UK2Node_MakeArray, UK2Node_MakeSet, UK2Node_MakeMap
- **Struct** - UK2Node_MakeStruct, UK2Node_BreakStruct
- **Enum** - UK2Node_EnumLiteral, UK2Node_SwitchEnum
- **Math** - UK2Node_CommutativeAssociativeBinaryOperator
- **Utilities** - UK2Node_Self, UK2Node_GetClassDefaults

---

## Step 3: Enhanced SerializeNode

**File:** `Source/BlueprintSerializerEditor/Private/BlueprintToJson.cpp`

### Node-Type-Specific Serialization

The `SerializeNode` function now handles 15+ node types with specialized fields:

```cpp
void FBlueprintToJson::SerializeNode(UK2Node* Node, FBlueprintJsonNode& OutNode)
{
    // Common fields
    OutNode.NodeClass = Node->GetClass()->GetName();
    OutNode.NodeGuid = Node->NodeGuid.ToString();
    // ... pins, position, comment ...

    // K2Node_CallFunction
    if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        const FMemberReference& FuncRef = CallFunc->FunctionReference;
        OutNode.FunctionReference.MemberName = FuncRef.GetMemberName().ToString();
        OutNode.FunctionReference.MemberParentClass = GetClassPath(FuncRef.GetMemberParentClass());
        OutNode.FunctionReference.bSelfContext = FuncRef.IsSelfContext();
        OutNode.bIsPure = CallFunc->bIsPureFunc;
    }
    
    // K2Node_Event
    else if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
    {
        OutNode.EventReference.MemberName = Event->EventReference.GetMemberName().ToString();
        // ...
    }
    
    // K2Node_VariableGet/Set
    else if (UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(Node))
    {
        OutNode.VariableReference.MemberName = VarNode->GetVarName().ToString();
        OutNode.VariableReference.bSelfContext = VarNode->VariableReference.IsSelfContext();
        // ...
    }
    
    // ... 12+ more node types ...
}
```

---

## Step 4: Fixed DeserializeNode

**File:** `Source/BlueprintSerializerEditor/Private/JsonToBlueprint.cpp`

### FMemberReference API

The correct API for setting member references:

```cpp
// External member (function on another class)
FMemberReference Ref;
Ref.SetExternalMember(FName(*MemberName), ParentClass);

// Self member (function on this Blueprint)
Ref.SetSelfMember(FName(*MemberName));

// Local member (variable with GUID)
Ref.SetLocalMember(FName(*MemberName), OwnerStruct, MemberGuid);
```

### Helper Functions

```cpp
// Resolve a serialized reference to a UFunction
UFunction* ResolveMemberReferenceAsFunction(
    const FBlueprintJsonMemberReference& Ref, 
    UBlueprint* Blueprint);

// Find function by full path (e.g., "/Script/Engine.Actor:GetActorLocation")
UFunction* ResolveFunctionFromPath(const FString& FunctionPath);

// Apply reference to a variable node
void ApplyMemberReferenceToVariable(
    UK2Node_Variable* VarNode,
    const FBlueprintJsonMemberReference& Ref,
    UBlueprint* Blueprint);
```

---

## Step 5: BlueprintJsonValidator

**Files:**
- `Source/BlueprintSerializerEditor/Public/BlueprintJsonValidator.h`
- `Source/BlueprintSerializerEditor/Private/BlueprintJsonValidator.cpp`

### Validation Issue

```cpp
struct FBlueprintJsonValidationIssue
{
    enum class ESeverity : uint8
    {
        Error,      // Will cause deserialization to fail
        Warning,    // May cause problems
        Info        // Informational
    };

    ESeverity Severity;
    FString NodeGuid;
    FString PropertyName;
    FString Message;
};
```

### Validation Result

```cpp
struct FBlueprintJsonValidationResult
{
    bool bIsValid = true;
    TArray<FBlueprintJsonValidationIssue> Issues;

    bool HasErrors() const;
    bool HasWarnings() const;
    FString ToString() const;
};
```

### Validator API

```cpp
class FBlueprintJsonValidator
{
public:
    // Validate parsed data
    static FBlueprintJsonValidationResult Validate(const FBlueprintJsonData& JsonData);
    
    // Validate JSON string
    static FBlueprintJsonValidationResult ValidateJsonString(const FString& JsonString);
    
    // Validate JSON file
    static FBlueprintJsonValidationResult ValidateJsonFile(const FString& FilePath);
    
    // Validate single node
    static void ValidateNode(const FBlueprintJsonNode& Node, 
                            TArray<FBlueprintJsonValidationIssue>& OutIssues);
    
    // Check if class path is resolvable
    static bool CanResolveClass(const FString& ClassPath);
};
```

---

## Step 6: Python Schema Module

**File:** `Scripts/blueprint_schema.py`

### Features

- **Type definitions** matching C++ structures
- **Validation utilities** for JSON data
- **Schema-aware editing** helpers
- **Transformation utilities** for batch operations

### Usage Example

```python
from blueprint_schema import BlueprintJson, validate_blueprint_json

# Load and validate
bp = BlueprintJson.from_file("BP_MyBlueprint.json")
issues = validate_blueprint_json(bp)

# Find nodes
call_nodes = bp.find_nodes_by_class("K2Node_CallFunction")

# Modify
for node in call_nodes:
    if node.function_reference.member_name == "PrintString":
        node.function_reference.member_name = "PrintWarning"

# Save
bp.save("BP_MyBlueprint_modified.json")
```

---

## Step 7: Editor Commands

**File:** `Source/BlueprintSerializerEditor/Private/BlueprintSerializerEditorModule.cpp`

### Content Browser Context Menu

Right-click any Blueprint in the Content Browser:

| Menu Item | Description |
|-----------|-------------|
| **Export to JSON** | Save Blueprint as JSON file |
| **Import from JSON...** | Merge JSON file into Blueprint |
| **Generate Schema** | Generate JSON schema file |
| **Validate JSON File...** | Validate a Blueprint JSON file |

### Console Commands

Execute in the Output Log or Editor Console:

```
BlueprintSerializer.GenerateNodeCatalog
```
Generates `Saved/BlueprintSerializer/node_catalog.json` with all K2Node types.

```
BlueprintSerializer.GenerateMasterSchema
```
Generates `Saved/BlueprintSerializer/blueprint_master_schema.json` with complete schema.

```
BlueprintSerializer.ValidateFile <path>
```
Validates a Blueprint JSON file and outputs results to log.

---

## JSON Format Example

```json
{
  "SchemaVersion": "2.0.0",
  "BlueprintName": "BP_MyActor",
  "BlueprintClass": "/Script/Engine.Actor",
  "Graphs": [
    {
      "GraphName": "EventGraph",
      "GraphType": "Function",
      "Nodes": [
        {
          "NodeClass": "K2Node_CallFunction",
          "NodeGuid": "A1B2C3D4-...",
          "FunctionReference": {
            "MemberName": "PrintString",
            "MemberParentClass": "/Script/Engine.KismetSystemLibrary",
            "bSelfContext": false
          },
          "bIsPure": false,
          "Pins": [
            {
              "PinName": "execute",
              "Direction": "Input",
              "PinType": "exec"
            },
            {
              "PinName": "InString",
              "Direction": "Input",
              "PinType": "string",
              "DefaultValue": "Hello World"
            }
          ]
        }
      ]
    }
  ],
  "Variables": [
    {
      "VariableName": "MyHealth",
      "VariableType": "float",
      "DefaultValue": "100.0"
    }
  ]
}
```

---

## Troubleshooting

### Common Validation Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "Unknown node class" | K2Node type not found | Check spelling, ensure module is loaded |
| "Cannot resolve class" | Class path invalid | Use full path: `/Script/Module.ClassName` |
| "Missing FunctionReference" | CallFunction without ref | Add FunctionReference field |
| "Invalid pin connection" | Pin GUID mismatch | Verify NodeGuid and PinId match |

### FMemberReference Errors

If function calls fail to deserialize:

1. Verify `MemberParentClass` is a valid class path
2. For self-context functions, set `bSelfContext: true`
3. For Blueprint functions, include `MemberGuid`

---

## Version History

| Version | Changes |
|---------|---------|
| 1.0.0 | Initial release with basic serialization |
| 2.0.0 | Added FMemberReference, node-specific fields, validation |
