// Copyright CattleGame. All Rights Reserved.

#include "BlueprintSchemaGenerator.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_SpawnActor.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_Timeline.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Switch.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_SwitchName.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_Select.h"
#include "K2Node_ForEachElementInEnum.h"
#include "K2Node_CastByteToEnum.h"
#include "K2Node_EnumLiteral.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_EnumInequality.h"
#include "K2Node_GetEnumeratorName.h"
#include "K2Node_GetEnumeratorNameAsString.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeSet.h"
#include "K2Node_MakeMap.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_SetFieldsInStruct.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_GetDataTableRow.h"
#include "K2Node_FormatText.h"
#include "K2Node_Knot.h"
#include "K2Node_Self.h"
#include "K2Node_Literal.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_AddComponent.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallMaterialParameterCollectionFunction.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "K2Node_DoOnceMultiInput.h"
#include "K2Node_EaseFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_FunctionTerminator.h"
#include "K2Node_Tunnel.h"
#include "K2Node_TunnelBoundary.h"
#include "K2Node_Composite.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputKey.h"
#include "K2Node_InputTouch.h"
#include "K2Node_InputAxisKeyEvent.h"
#include "K2Node_InputVectorAxisEvent.h"
#include "K2Node_LoadAsset.h"
#include "K2Node_Message.h"
#include "K2Node_MultiGate.h"
#include "K2Node_ClassDynamicCast.h"
#include "K2Node_PromotableOperator.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_BaseAsyncTask.h"
#include "UObject/UObjectIterator.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"

TOptional<FBlueprintSerializerSchema> FBlueprintSchemaGenerator::CachedSchema;

FBlueprintSerializerSchema FBlueprintSchemaGenerator::GenerateFullSchema()
{
    FBlueprintSerializerSchema Schema;
    Schema.EngineVersion = FEngineVersion::Current().ToString();
    Schema.SchemaVersion = TEXT("2.0.0");

    // Enumerate all UK2Node subclasses using reflection
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass *Class = *ClassIt;

        // Skip abstract classes and the base UK2Node itself
        if (Class->HasAnyClassFlags(CLASS_Abstract))
        {
            continue;
        }

        // Check if this is a K2Node subclass
        if (!Class->IsChildOf(UK2Node::StaticClass()))
        {
            continue;
        }

        // Skip UK2Node itself
        if (Class == UK2Node::StaticClass())
        {
            continue;
        }

        FBlueprintNodeSchema NodeSchema = GenerateNodeSchema(Class);
        Schema.NodeSchemas.Add(NodeSchema);
    }

    // Sort by category then by display name for readability
    Schema.NodeSchemas.Sort([](const FBlueprintNodeSchema &A, const FBlueprintNodeSchema &B)
                            {
        if (A.Category != B.Category)
        {
            return A.Category < B.Category;
        }
        return A.DisplayName < B.DisplayName; });

    UE_LOG(LogTemp, Log, TEXT("BlueprintSchemaGenerator: Generated schema with %d node types"), Schema.NodeSchemas.Num());

    return Schema;
}

FBlueprintNodeSchema FBlueprintSchemaGenerator::GenerateNodeSchema(UClass *NodeClass)
{
    FBlueprintNodeSchema Schema;

    if (!NodeClass)
    {
        return Schema;
    }

    Schema.NodeClass = NodeClass->GetName();
    Schema.DisplayName = GetNodeDisplayName(NodeClass);
    Schema.Category = GetNodeCategory(NodeClass);
    Schema.Description = GetNodeDescription(NodeClass);
    Schema.bCanBePure = CanNodeBePure(NodeClass);
    Schema.bIsLatent = IsNodeLatent(NodeClass);

    // Base node properties that all nodes have
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NodeClass");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("The K2Node class name (e.g., 'K2Node_CallFunction')");
        Schema.Properties.Add(Prop);
    }
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NodeGuid");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Unique identifier for this node instance");
        Schema.Properties.Add(Prop);
    }
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NodePosX");
        Prop.PropertyType = TEXT("integer");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("X position in the graph editor");
        Schema.Properties.Add(Prop);
    }
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NodePosY");
        Prop.PropertyType = TEXT("integer");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Y position in the graph editor");
        Schema.Properties.Add(Prop);
    }
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NodeComment");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Developer comment for this node");
        Schema.Properties.Add(Prop);
    }

    // Extract node-specific properties
    ExtractNodeSpecificProperties(NodeClass, Schema);

    return Schema;
}

TArray<UClass *> FBlueprintSchemaGenerator::GetAllK2NodeClasses()
{
    TArray<UClass *> NodeClasses;

    // Enumerate all UK2Node subclasses using reflection
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass *Class = *ClassIt;

        // Skip abstract classes and the base UK2Node itself
        if (Class->HasAnyClassFlags(CLASS_Abstract))
        {
            continue;
        }

        // Check if this is a K2Node subclass
        if (!Class->IsChildOf(UK2Node::StaticClass()))
        {
            continue;
        }

        // Skip UK2Node itself
        if (Class == UK2Node::StaticClass())
        {
            continue;
        }

        NodeClasses.Add(Class);
    }

    // Sort by class name for consistency
    Algo::Sort(NodeClasses, [](UClass *A, UClass *B)
               { return A->GetName() < B->GetName(); });

    return NodeClasses;
}

FString FBlueprintSchemaGenerator::GetNodeCategory(UClass *NodeClass)
{
    if (!NodeClass)
    {
        return TEXT("Unknown");
    }

    // Check parent classes to determine category
    if (NodeClass->IsChildOf(UK2Node_CallFunction::StaticClass()))
    {
        return TEXT("Function Calls");
    }
    if (NodeClass->IsChildOf(UK2Node_Event::StaticClass()))
    {
        return TEXT("Events");
    }
    if (NodeClass->IsChildOf(UK2Node_VariableGet::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_VariableSet::StaticClass()))
    {
        return TEXT("Variables");
    }
    if (NodeClass->IsChildOf(UK2Node_DynamicCast::StaticClass()))
    {
        return TEXT("Casting");
    }
    if (NodeClass->IsChildOf(UK2Node_Switch::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_IfThenElse::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_Select::StaticClass()))
    {
        return TEXT("Flow Control");
    }
    if (NodeClass->IsChildOf(UK2Node_MakeArray::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_MakeSet::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_MakeMap::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_GetArrayItem::StaticClass()))
    {
        return TEXT("Containers");
    }
    if (NodeClass->IsChildOf(UK2Node_MakeStruct::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_BreakStruct::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_SetFieldsInStruct::StaticClass()))
    {
        return TEXT("Struct");
    }
    if (NodeClass->IsChildOf(UK2Node_EnumLiteral::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_SwitchEnum::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_CastByteToEnum::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_ForEachElementInEnum::StaticClass()))
    {
        return TEXT("Enum");
    }
    if (NodeClass->IsChildOf(UK2Node_CreateDelegate::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_CallDelegate::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_AddDelegate::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_RemoveDelegate::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_ClearDelegate::StaticClass()))
    {
        return TEXT("Delegates");
    }
    if (NodeClass->IsChildOf(UK2Node_Timeline::StaticClass()))
    {
        return TEXT("Timeline");
    }
    if (NodeClass->IsChildOf(UK2Node_MacroInstance::StaticClass()))
    {
        return TEXT("Macros");
    }
    if (NodeClass->IsChildOf(UK2Node_SpawnActorFromClass::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_ConstructObjectFromClass::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_AddComponent::StaticClass()))
    {
        return TEXT("Spawning");
    }
    if (NodeClass->IsChildOf(UK2Node_InputAction::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_InputKey::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_InputTouch::StaticClass()))
    {
        return TEXT("Input");
    }
    if (NodeClass->IsChildOf(UK2Node_Tunnel::StaticClass()))
    {
        return TEXT("Tunnel");
    }
    if (NodeClass->IsChildOf(UK2Node_FunctionEntry::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_FunctionResult::StaticClass()))
    {
        return TEXT("Function Definition");
    }
    if (NodeClass->IsChildOf(UK2Node_Knot::StaticClass()))
    {
        return TEXT("Utility");
    }
    if (NodeClass->IsChildOf(UK2Node_Self::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_Literal::StaticClass()))
    {
        return TEXT("Literals");
    }
    if (NodeClass->IsChildOf(UK2Node_AsyncAction::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_BaseAsyncTask::StaticClass()))
    {
        return TEXT("Async/Latent");
    }

    return TEXT("Other");
}

bool FBlueprintSchemaGenerator::CanNodeBePure(UClass *NodeClass)
{
    if (!NodeClass)
    {
        return false;
    }

    // Check if this node type can be pure by checking for pure-related properties
    if (NodeClass->IsChildOf(UK2Node_CallFunction::StaticClass()))
    {
        return true; // CallFunction nodes can be pure depending on the function
    }
    if (NodeClass->IsChildOf(UK2Node_VariableGet::StaticClass()))
    {
        return true;
    }
    if (NodeClass->IsChildOf(UK2Node_MakeStruct::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_BreakStruct::StaticClass()))
    {
        return true;
    }
    if (NodeClass->IsChildOf(UK2Node_MakeArray::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_MakeSet::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_MakeMap::StaticClass()))
    {
        return true;
    }
    if (NodeClass->IsChildOf(UK2Node_DynamicCast::StaticClass()))
    {
        return true; // Can be pure cast
    }

    return false;
}

bool FBlueprintSchemaGenerator::IsNodeLatent(UClass *NodeClass)
{
    if (!NodeClass)
    {
        return false;
    }

    if (NodeClass->IsChildOf(UK2Node_BaseAsyncTask::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_AsyncAction::StaticClass()) ||
        NodeClass->IsChildOf(UK2Node_LoadAsset::StaticClass()))
    {
        return true;
    }

    // Timeline is implicitly latent
    if (NodeClass->IsChildOf(UK2Node_Timeline::StaticClass()))
    {
        return true;
    }

    return false;
}

void FBlueprintSchemaGenerator::ExtractNodeSpecificProperties(UClass *NodeClass, FBlueprintNodeSchema &OutSchema)
{
    if (!NodeClass)
    {
        return;
    }

    // K2Node_CallFunction - needs FunctionReference
    if (NodeClass->IsChildOf(UK2Node_CallFunction::StaticClass()))
    {
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("FunctionReference");
            Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Reference to the function being called");
            OutSchema.Properties.Add(Prop);
        }
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("IsNodePure");
            Prop.PropertyType = TEXT("boolean");
            Prop.Requirement = EBlueprintSchemaRequirement::Optional;
            Prop.Description = TEXT("Whether this is a pure function call (no exec pins)");
            Prop.DefaultValue = TEXT("false");
            OutSchema.Properties.Add(Prop);
        }
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_Event, K2Node_CustomEvent - needs EventReference
    else if (NodeClass->IsChildOf(UK2Node_Event::StaticClass()))
    {
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("EventReference");
            Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Reference to the event signature");
            OutSchema.Properties.Add(Prop);
        }
        if (NodeClass == UK2Node_CustomEvent::StaticClass() ||
            NodeClass->IsChildOf(UK2Node_CustomEvent::StaticClass()))
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("CustomFunctionName");
            Prop.PropertyType = TEXT("string");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Name of the custom event");
            OutSchema.Properties.Add(Prop);
        }
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_VariableGet, K2Node_VariableSet - needs VariableReference
    else if (NodeClass->IsChildOf(UK2Node_VariableGet::StaticClass()) ||
             NodeClass->IsChildOf(UK2Node_VariableSet::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("VariableReference");
        Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Reference to the variable");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_DynamicCast, K2Node_ClassDynamicCast - needs TargetClass
    else if (NodeClass->IsChildOf(UK2Node_DynamicCast::StaticClass()))
    {
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("TargetClass");
            Prop.PropertyType = TEXT("string");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Class path to cast to (e.g., '/Script/Engine.Actor')");
            OutSchema.Properties.Add(Prop);
        }
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("IsPureCast");
            Prop.PropertyType = TEXT("boolean");
            Prop.Requirement = EBlueprintSchemaRequirement::Optional;
            Prop.Description = TEXT("Whether this is a pure cast (no exec pins)");
            Prop.DefaultValue = TEXT("false");
            OutSchema.Properties.Add(Prop);
        }
    }
    // K2Node_SpawnActorFromClass - needs SpawnClass
    else if (NodeClass->IsChildOf(UK2Node_SpawnActorFromClass::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("SpawnClass");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Class path of actor to spawn (can also be set via input pin)");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_ConstructObjectFromClass
    else if (NodeClass->IsChildOf(UK2Node_ConstructObjectFromClass::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("SpawnClass");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Class path of object to construct");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_Timeline - needs TimelineName
    else if (NodeClass->IsChildOf(UK2Node_Timeline::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("TimelineName");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Name of the timeline in this Blueprint");
        OutSchema.Properties.Add(Prop);
        OutSchema.bIsLatent = true;
    }
    // K2Node_MacroInstance - needs MacroReference
    else if (NodeClass->IsChildOf(UK2Node_MacroInstance::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("MacroReference");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Path to the macro graph asset");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_SwitchEnum - needs EnumType
    else if (NodeClass->IsChildOf(UK2Node_SwitchEnum::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("EnumType");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Path to the enum type");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_EnumLiteral
    else if (NodeClass->IsChildOf(UK2Node_EnumLiteral::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("EnumType");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Path to the enum type");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_MakeStruct, K2Node_BreakStruct, K2Node_SetFieldsInStruct
    else if (NodeClass->IsChildOf(UK2Node_MakeStruct::StaticClass()) ||
             NodeClass->IsChildOf(UK2Node_BreakStruct::StaticClass()) ||
             NodeClass->IsChildOf(UK2Node_SetFieldsInStruct::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("StructType");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Path to the struct type (e.g., '/Script/CoreUObject.Vector')");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_CreateDelegate
    else if (NodeClass->IsChildOf(UK2Node_CreateDelegate::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("DelegateReference");
        Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Reference to the function to bind to delegate");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_CallDelegate
    else if (NodeClass->IsChildOf(UK2Node_CallDelegate::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("DelegateReference");
        Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Reference to the delegate property");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_AddDelegate, K2Node_RemoveDelegate, K2Node_ClearDelegate
    else if (NodeClass->IsChildOf(UK2Node_AddDelegate::StaticClass()) ||
             NodeClass->IsChildOf(UK2Node_RemoveDelegate::StaticClass()) ||
             NodeClass->IsChildOf(UK2Node_ClearDelegate::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("DelegateReference");
        Prop.PropertyType = TEXT("FBlueprintJsonMemberReference");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Reference to the delegate property");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_InputAction
    else if (NodeClass->IsChildOf(UK2Node_InputAction::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("InputActionName");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Name of the input action");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_InputKey
    else if (NodeClass->IsChildOf(UK2Node_InputKey::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("InputKey");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("The input key (e.g., 'SpaceBar', 'LeftMouseButton')");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_FormatText
    else if (NodeClass->IsChildOf(UK2Node_FormatText::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("FormatString");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Format string with {Arg} placeholders");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_GetDataTableRow
    else if (NodeClass->IsChildOf(UK2Node_GetDataTableRow::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("DataTable");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Path to the data table asset");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_ExecutionSequence
    else if (NodeClass->IsChildOf(UK2Node_ExecutionSequence::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("NumOutputPins");
        Prop.PropertyType = TEXT("integer");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Number of execution output pins");
        Prop.DefaultValue = TEXT("2");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_MultiGate
    else if (NodeClass->IsChildOf(UK2Node_MultiGate::StaticClass()))
    {
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("NumOutputPins");
            Prop.PropertyType = TEXT("integer");
            Prop.Requirement = EBlueprintSchemaRequirement::Optional;
            Prop.Description = TEXT("Number of gate output pins");
            Prop.DefaultValue = TEXT("2");
            OutSchema.Properties.Add(Prop);
        }
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("bIsRandom");
            Prop.PropertyType = TEXT("boolean");
            Prop.Requirement = EBlueprintSchemaRequirement::Optional;
            Prop.Description = TEXT("Whether to select random output");
            Prop.DefaultValue = TEXT("false");
            OutSchema.Properties.Add(Prop);
        }
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("bLoop");
            Prop.PropertyType = TEXT("boolean");
            Prop.Requirement = EBlueprintSchemaRequirement::Optional;
            Prop.Description = TEXT("Whether to loop through outputs");
            Prop.DefaultValue = TEXT("false");
            OutSchema.Properties.Add(Prop);
        }
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_Literal
    else if (NodeClass->IsChildOf(UK2Node_Literal::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("LiteralValue");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("The literal value (object reference path)");
        OutSchema.Properties.Add(Prop);
    }
    // K2Node_TemporaryVariable
    else if (NodeClass->IsChildOf(UK2Node_TemporaryVariable::StaticClass()))
    {
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("VariableName");
            Prop.PropertyType = TEXT("string");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Name of the local variable");
            OutSchema.Properties.Add(Prop);
        }
        {
            FBlueprintNodeSchemaProperty Prop;
            Prop.PropertyName = TEXT("VariableType");
            Prop.PropertyType = TEXT("string");
            Prop.Requirement = EBlueprintSchemaRequirement::Required;
            Prop.Description = TEXT("Type of the variable");
            OutSchema.Properties.Add(Prop);
        }
    }
    // K2Node_FunctionEntry
    else if (NodeClass->IsChildOf(UK2Node_FunctionEntry::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("FunctionName");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Computed;
        Prop.Description = TEXT("Name of the function (for custom functions)");
        OutSchema.Properties.Add(Prop);
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_FunctionResult
    else if (NodeClass->IsChildOf(UK2Node_FunctionResult::StaticClass()))
    {
        OutSchema.bHasDynamicPins = true;
    }
    // K2Node_LoadAsset
    else if (NodeClass->IsChildOf(UK2Node_LoadAsset::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("AssetClass");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
        Prop.Description = TEXT("Class of asset to load");
        OutSchema.Properties.Add(Prop);
        OutSchema.bIsLatent = true;
    }
    // K2Node_AsyncAction
    else if (NodeClass->IsChildOf(UK2Node_AsyncAction::StaticClass()))
    {
        FBlueprintNodeSchemaProperty Prop;
        Prop.PropertyName = TEXT("ProxyClass");
        Prop.PropertyType = TEXT("string");
        Prop.Requirement = EBlueprintSchemaRequirement::Required;
        Prop.Description = TEXT("Class of the async action proxy");
        OutSchema.Properties.Add(Prop);
        OutSchema.bIsLatent = true;
        OutSchema.bHasDynamicPins = true;
    }
}

FString FBlueprintSchemaGenerator::GetNodeDisplayName(UClass *NodeClass)
{
    if (!NodeClass)
    {
        return TEXT("Unknown");
    }

    FString ClassName = NodeClass->GetName();

    // Strip "K2Node_" prefix for display
    if (ClassName.StartsWith(TEXT("K2Node_")))
    {
        ClassName = ClassName.RightChop(7);
    }

    // Convert PascalCase to spaced words
    FString DisplayName;
    for (int32 i = 0; i < ClassName.Len(); ++i)
    {
        TCHAR Char = ClassName[i];
        if (i > 0 && FChar::IsUpper(Char) && !FChar::IsUpper(ClassName[i - 1]))
        {
            DisplayName += TEXT(" ");
        }
        DisplayName += Char;
    }

    return DisplayName;
}

FString FBlueprintSchemaGenerator::GetNodeDescription(UClass *NodeClass)
{
    if (!NodeClass)
    {
        return TEXT("");
    }

    // Try to get the tooltip metadata if available
    FString Tooltip = NodeClass->GetMetaData(TEXT("Tooltip"));
    if (!Tooltip.IsEmpty())
    {
        return Tooltip;
    }

    // Provide default descriptions for common node types
    FString ClassName = NodeClass->GetName();

    if (ClassName == TEXT("K2Node_CallFunction"))
    {
        return TEXT("Calls a function on an object or class.");
    }
    if (ClassName == TEXT("K2Node_Event"))
    {
        return TEXT("Entry point for an event in the Blueprint.");
    }
    if (ClassName == TEXT("K2Node_CustomEvent"))
    {
        return TEXT("A custom event that can be called from other Blueprints.");
    }
    if (ClassName == TEXT("K2Node_VariableGet"))
    {
        return TEXT("Gets the value of a variable.");
    }
    if (ClassName == TEXT("K2Node_VariableSet"))
    {
        return TEXT("Sets the value of a variable.");
    }
    if (ClassName == TEXT("K2Node_IfThenElse") || ClassName == TEXT("K2Node_Branch"))
    {
        return TEXT("Conditional branch - executes one path based on a boolean condition.");
    }
    if (ClassName == TEXT("K2Node_DynamicCast"))
    {
        return TEXT("Casts an object to a different type.");
    }
    if (ClassName == TEXT("K2Node_SpawnActorFromClass"))
    {
        return TEXT("Spawns an actor of the specified class in the world.");
    }
    if (ClassName == TEXT("K2Node_Timeline"))
    {
        return TEXT("Plays a timeline for interpolating values over time.");
    }

    return FString::Printf(TEXT("A %s node."), *GetNodeDisplayName(NodeClass));
}

bool FBlueprintSchemaGenerator::ExportSchemaToFile(const FBlueprintSerializerSchema &Schema, const FString &FilePath)
{
    FString JsonString;
    if (!ExportSchemaToString(Schema, JsonString))
    {
        return false;
    }

    if (!FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintSchemaGenerator: Failed to write schema to file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("BlueprintSchemaGenerator: Exported schema to: %s"), *FilePath);
    return true;
}

bool FBlueprintSchemaGenerator::ExportSchemaToString(const FBlueprintSerializerSchema &Schema, FString &OutJsonString)
{
    TSharedRef<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    RootObject->SetStringField(TEXT("engineVersion"), Schema.EngineVersion);
    RootObject->SetStringField(TEXT("schemaVersion"), Schema.SchemaVersion);

    TArray<TSharedPtr<FJsonValue>> NodeSchemasArray;
    for (const FBlueprintNodeSchema &NodeSchema : Schema.NodeSchemas)
    {
        TSharedRef<FJsonObject> NodeObject = MakeShareable(new FJsonObject);

        NodeObject->SetStringField(TEXT("nodeClass"), NodeSchema.NodeClass);
        NodeObject->SetStringField(TEXT("displayName"), NodeSchema.DisplayName);
        NodeObject->SetStringField(TEXT("category"), NodeSchema.Category);

        if (!NodeSchema.Description.IsEmpty())
        {
            NodeObject->SetStringField(TEXT("description"), NodeSchema.Description);
        }

        NodeObject->SetBoolField(TEXT("hasDynamicPins"), NodeSchema.bHasDynamicPins);
        NodeObject->SetBoolField(TEXT("isLatent"), NodeSchema.bIsLatent);
        NodeObject->SetBoolField(TEXT("canBePure"), NodeSchema.bCanBePure);

        TArray<TSharedPtr<FJsonValue>> PropertiesArray;
        for (const FBlueprintNodeSchemaProperty &Prop : NodeSchema.Properties)
        {
            TSharedRef<FJsonObject> PropObject = MakeShareable(new FJsonObject);

            PropObject->SetStringField(TEXT("name"), Prop.PropertyName);
            PropObject->SetStringField(TEXT("type"), Prop.PropertyType);

            FString RequirementStr;
            switch (Prop.Requirement)
            {
            case EBlueprintSchemaRequirement::Required:
                RequirementStr = TEXT("required");
                break;
            case EBlueprintSchemaRequirement::Optional:
                RequirementStr = TEXT("optional");
                break;
            case EBlueprintSchemaRequirement::Computed:
                RequirementStr = TEXT("computed");
                break;
            }
            PropObject->SetStringField(TEXT("requirement"), RequirementStr);

            if (!Prop.Description.IsEmpty())
            {
                PropObject->SetStringField(TEXT("description"), Prop.Description);
            }
            if (!Prop.DefaultValue.IsEmpty())
            {
                PropObject->SetStringField(TEXT("defaultValue"), Prop.DefaultValue);
            }

            PropertiesArray.Add(MakeShareable(new FJsonValueObject(PropObject)));
        }
        NodeObject->SetArrayField(TEXT("properties"), PropertiesArray);

        NodeSchemasArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
    }

    RootObject->SetArrayField(TEXT("nodeSchemas"), NodeSchemasArray);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
    if (!FJsonSerializer::Serialize(RootObject, Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintSchemaGenerator: Failed to serialize schema to JSON"));
        return false;
    }

    return true;
}

const FBlueprintSerializerSchema &FBlueprintSchemaGenerator::GetCachedSchema()
{
    if (!CachedSchema.IsSet())
    {
        FBlueprintSerializerSchema Schema;

        // Try to load from cache file first
        if (LoadSchemaFromCache(Schema))
        {
            // Validate the cached schema version matches current engine
            if (Schema.EngineVersion == FEngineVersion::Current().ToString())
            {
                CachedSchema = Schema;
                UE_LOG(LogTemp, Log, TEXT("BlueprintSchemaGenerator: Loaded schema from cache"));
            }
            else
            {
                // Engine version mismatch, regenerate
                UE_LOG(LogTemp, Log, TEXT("BlueprintSchemaGenerator: Cache engine version mismatch, regenerating"));
                CachedSchema = GenerateFullSchema();
                SaveSchemaToCache(CachedSchema.GetValue());
            }
        }
        else
        {
            // No valid cache, generate new
            CachedSchema = GenerateFullSchema();
            SaveSchemaToCache(CachedSchema.GetValue());
        }
    }

    return CachedSchema.GetValue();
}

void FBlueprintSchemaGenerator::InvalidateCache()
{
    CachedSchema.Reset();

    // Also delete cache file
    FString CachePath = GetDefaultSchemaFilePath();
    IFileManager::Get().Delete(*CachePath);

    UE_LOG(LogTemp, Log, TEXT("BlueprintSchemaGenerator: Cache invalidated"));
}

FString FBlueprintSchemaGenerator::GetDefaultSchemaFilePath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BlueprintSerializer"), TEXT("NodeSchema.json"));
}

bool FBlueprintSchemaGenerator::LoadSchemaFromCache(FBlueprintSerializerSchema &OutSchema)
{
    FString CachePath = GetDefaultSchemaFilePath();

    if (!FPaths::FileExists(CachePath))
    {
        return false;
    }

    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *CachePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        return false;
    }

    OutSchema.EngineVersion = RootObject->GetStringField(TEXT("engineVersion"));
    OutSchema.SchemaVersion = RootObject->GetStringField(TEXT("schemaVersion"));

    const TArray<TSharedPtr<FJsonValue>> *NodeSchemasArray;
    if (RootObject->TryGetArrayField(TEXT("nodeSchemas"), NodeSchemasArray))
    {
        for (const TSharedPtr<FJsonValue> &NodeValue : *NodeSchemasArray)
        {
            const TSharedPtr<FJsonObject> *NodeObject;
            if (!NodeValue->TryGetObject(NodeObject))
            {
                continue;
            }

            FBlueprintNodeSchema NodeSchema;
            NodeSchema.NodeClass = (*NodeObject)->GetStringField(TEXT("nodeClass"));
            NodeSchema.DisplayName = (*NodeObject)->GetStringField(TEXT("displayName"));
            NodeSchema.Category = (*NodeObject)->GetStringField(TEXT("category"));
            (*NodeObject)->TryGetStringField(TEXT("description"), NodeSchema.Description);
            (*NodeObject)->TryGetBoolField(TEXT("hasDynamicPins"), NodeSchema.bHasDynamicPins);
            (*NodeObject)->TryGetBoolField(TEXT("isLatent"), NodeSchema.bIsLatent);
            (*NodeObject)->TryGetBoolField(TEXT("canBePure"), NodeSchema.bCanBePure);

            const TArray<TSharedPtr<FJsonValue>> *PropertiesArray;
            if ((*NodeObject)->TryGetArrayField(TEXT("properties"), PropertiesArray))
            {
                for (const TSharedPtr<FJsonValue> &PropValue : *PropertiesArray)
                {
                    const TSharedPtr<FJsonObject> *PropObject;
                    if (!PropValue->TryGetObject(PropObject))
                    {
                        continue;
                    }

                    FBlueprintNodeSchemaProperty Prop;
                    Prop.PropertyName = (*PropObject)->GetStringField(TEXT("name"));
                    Prop.PropertyType = (*PropObject)->GetStringField(TEXT("type"));

                    FString RequirementStr = (*PropObject)->GetStringField(TEXT("requirement"));
                    if (RequirementStr == TEXT("required"))
                    {
                        Prop.Requirement = EBlueprintSchemaRequirement::Required;
                    }
                    else if (RequirementStr == TEXT("optional"))
                    {
                        Prop.Requirement = EBlueprintSchemaRequirement::Optional;
                    }
                    else
                    {
                        Prop.Requirement = EBlueprintSchemaRequirement::Computed;
                    }

                    (*PropObject)->TryGetStringField(TEXT("description"), Prop.Description);
                    (*PropObject)->TryGetStringField(TEXT("defaultValue"), Prop.DefaultValue);

                    NodeSchema.Properties.Add(Prop);
                }
            }

            OutSchema.NodeSchemas.Add(NodeSchema);
        }
    }

    return true;
}

bool FBlueprintSchemaGenerator::SaveSchemaToCache(const FBlueprintSerializerSchema &Schema)
{
    FString CachePath = GetDefaultSchemaFilePath();

    // Ensure directory exists
    FString CacheDir = FPaths::GetPath(CachePath);
    if (!IFileManager::Get().DirectoryExists(*CacheDir))
    {
        IFileManager::Get().MakeDirectory(*CacheDir, true);
    }

    return ExportSchemaToFile(Schema, CachePath);
}
