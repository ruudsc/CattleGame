// Copyright CattleGame. All Rights Reserved.

#include "BlueprintToJson.h"
#include "BlueprintJsonFormat.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_Timeline.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_EnumLiteral.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_SetFieldsInStruct.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputKey.h"
#include "K2Node_FormatText.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MultiGate.h"
#include "K2Node_Literal.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_LoadAsset.h"
#include "K2Node_GetDataTableRow.h"
#include "K2Node_BaseAsyncTask.h"
#include "EdGraphSchema_K2.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Kismet2/BlueprintEditorUtils.h"

bool FBlueprintToJson::SerializeBlueprint(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    if (!Blueprint)
    {
        return false;
    }

    // Serialize all components
    SerializeMetadata(Blueprint, OutJsonData);
    SerializeVariables(Blueprint, OutJsonData);
    SerializeEventGraphs(Blueprint, OutJsonData);
    SerializeFunctions(Blueprint, OutJsonData);
    SerializeMacros(Blueprint, OutJsonData);
    SerializeComponents(Blueprint, OutJsonData);
    SerializeInterfaces(Blueprint, OutJsonData);
    SerializeDependencies(Blueprint, OutJsonData);

    return true;
}

bool FBlueprintToJson::SerializeBlueprintToString(const UBlueprint *Blueprint, FString &OutJsonString, bool bPrettyPrint)
{
    FBlueprintJsonData JsonData;
    if (!SerializeBlueprint(Blueprint, JsonData))
    {
        return false;
    }

    // Convert to JSON string
    if (bPrettyPrint)
    {
        return FJsonObjectConverter::UStructToJsonObjectString(JsonData, OutJsonString, 0, 0, 0, nullptr, true);
    }
    else
    {
        return FJsonObjectConverter::UStructToJsonObjectString(JsonData, OutJsonString);
    }
}

bool FBlueprintToJson::SerializeBlueprintToFile(const UBlueprint *Blueprint, const FString &FilePath, bool bPrettyPrint)
{
    FString JsonString;
    if (!SerializeBlueprintToString(Blueprint, JsonString, bPrettyPrint))
    {
        return false;
    }

    return FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void FBlueprintToJson::SerializeMetadata(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    FBlueprintJsonMetadata &Metadata = OutJsonData.Metadata;

    Metadata.BlueprintName = Blueprint->GetName();
    Metadata.BlueprintPath = Blueprint->GetPathName();

    // Blueprint type
    switch (Blueprint->BlueprintType)
    {
    case BPTYPE_Normal:
        Metadata.BlueprintType = TEXT("Normal");
        break;
    case BPTYPE_Const:
        Metadata.BlueprintType = TEXT("Const");
        break;
    case BPTYPE_MacroLibrary:
        Metadata.BlueprintType = TEXT("MacroLibrary");
        break;
    case BPTYPE_Interface:
        Metadata.BlueprintType = TEXT("Interface");
        break;
    case BPTYPE_LevelScript:
        Metadata.BlueprintType = TEXT("LevelScript");
        break;
    case BPTYPE_FunctionLibrary:
        Metadata.BlueprintType = TEXT("FunctionLibrary");
        break;
    default:
        Metadata.BlueprintType = TEXT("Unknown");
        break;
    }

    // Parent class
    if (Blueprint->ParentClass)
    {
        Metadata.ParentClass = Blueprint->ParentClass->GetPathName();
    }

    // Generated class
    if (Blueprint->GeneratedClass)
    {
        Metadata.GeneratedClass = Blueprint->GeneratedClass->GetName();
    }

    // Version info
    Metadata.EngineVersion = FApp::GetBuildVersion();
    Metadata.SerializerVersion = BLUEPRINT_SERIALIZER_VERSION;
    Metadata.ExportTimestamp = FDateTime::Now().ToString();
}

void FBlueprintToJson::SerializeVariables(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    for (const FBPVariableDescription &VarDesc : Blueprint->NewVariables)
    {
        FBlueprintJsonVariable JsonVar;
        JsonVar.VarName = VarDesc.VarName.ToString();
        JsonVar.VarGuid = VarDesc.VarGuid.ToString();
        JsonVar.VarType = PinTypeToString(VarDesc.VarType);
        JsonVar.Category = VarDesc.Category.ToString();
        JsonVar.DefaultValue = VarDesc.DefaultValue;
        JsonVar.bIsExposed = (VarDesc.PropertyFlags & CPF_ExposeOnSpawn) != 0;
        JsonVar.bIsReadOnly = (VarDesc.PropertyFlags & CPF_BlueprintReadOnly) != 0;

        // Replication
        if (VarDesc.PropertyFlags & CPF_Net)
        {
            JsonVar.ReplicationCondition = TEXT("Replicated");
        }
        else if (VarDesc.PropertyFlags & CPF_RepNotify)
        {
            JsonVar.ReplicationCondition = TEXT("RepNotify");
        }

        // Metadata
        for (const FBPVariableMetaDataEntry &MetaEntry : VarDesc.MetaDataArray)
        {
            JsonVar.Metadata.Add(MetaEntry.DataKey.ToString(), MetaEntry.DataValue);
        }

        OutJsonData.Variables.Add(JsonVar);
    }
}

void FBlueprintToJson::SerializeEventGraphs(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    for (UEdGraph *Graph : Blueprint->UbergraphPages)
    {
        if (Graph)
        {
            FBlueprintJsonGraph JsonGraph;
            JsonGraph.GraphType = TEXT("EventGraph");
            SerializeGraph(Graph, JsonGraph);
            OutJsonData.EventGraphs.Add(JsonGraph);
        }
    }
}

void FBlueprintToJson::SerializeFunctions(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    for (UEdGraph *Graph : Blueprint->FunctionGraphs)
    {
        if (Graph)
        {
            FBlueprintJsonFunction JsonFunc;
            JsonFunc.FunctionName = Graph->GetName();
            JsonFunc.FunctionGuid = Graph->GraphGuid.ToString();
            JsonFunc.Graph.GraphType = TEXT("Function");
            SerializeGraph(Graph, JsonFunc.Graph);

            // TODO: Extract function parameters and return values from entry/result nodes

            OutJsonData.Functions.Add(JsonFunc);
        }
    }
}

void FBlueprintToJson::SerializeMacros(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    for (UEdGraph *Graph : Blueprint->MacroGraphs)
    {
        if (Graph)
        {
            FBlueprintJsonGraph JsonGraph;
            JsonGraph.GraphType = TEXT("Macro");
            SerializeGraph(Graph, JsonGraph);
            OutJsonData.Macros.Add(JsonGraph);
        }
    }
}

void FBlueprintToJson::SerializeComponents(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    if (Blueprint->SimpleConstructionScript)
    {
        const TArray<USCS_Node *> &AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
        for (USCS_Node *Node : AllNodes)
        {
            if (Node && Node->ComponentTemplate)
            {
                FBlueprintJsonComponent JsonComp;
                JsonComp.ComponentName = Node->GetVariableName().ToString();
                JsonComp.ComponentClass = Node->ComponentTemplate->GetClass()->GetPathName();

                if (Node->ParentComponentOrVariableName != NAME_None)
                {
                    JsonComp.ParentComponent = Node->ParentComponentOrVariableName.ToString();
                }

                // TODO: Serialize component properties

                OutJsonData.Components.Add(JsonComp);
            }
        }
    }
}

void FBlueprintToJson::SerializeInterfaces(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    for (const FBPInterfaceDescription &Interface : Blueprint->ImplementedInterfaces)
    {
        if (Interface.Interface)
        {
            OutJsonData.ImplementedInterfaces.Add(Interface.Interface->GetPathName());
        }
    }
}

void FBlueprintToJson::SerializeDependencies(const UBlueprint *Blueprint, FBlueprintJsonData &OutJsonData)
{
    // TODO: Implement dependency tracking
    // This would require analyzing all object references in the Blueprint
}

void FBlueprintToJson::SerializeGraph(const UEdGraph *Graph, FBlueprintJsonGraph &OutGraphData)
{
    if (!Graph)
    {
        return;
    }

    OutGraphData.GraphName = Graph->GetName();
    OutGraphData.GraphGuid = Graph->GraphGuid.ToString();

    for (UEdGraphNode *Node : Graph->Nodes)
    {
        if (Node)
        {
            FBlueprintJsonNode JsonNode;
            SerializeNode(Node, JsonNode);
            OutGraphData.Nodes.Add(JsonNode);
        }
    }
}

void FBlueprintToJson::SerializeNode(const UEdGraphNode *Node, FBlueprintJsonNode &OutNodeData)
{
    if (!Node)
    {
        return;
    }

    OutNodeData.NodeGuid = Node->NodeGuid.ToString();
    OutNodeData.NodeClass = Node->GetClass()->GetName();
    OutNodeData.NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    OutNodeData.NodeComment = Node->NodeComment;
    OutNodeData.PositionX = Node->NodePosX;
    OutNodeData.PositionY = Node->NodePosY;

    // Serialize pins
    for (UEdGraphPin *Pin : Node->Pins)
    {
        if (Pin)
        {
            FBlueprintJsonPin JsonPin;
            SerializePin(Pin, JsonPin);
            OutNodeData.Pins.Add(JsonPin);
        }
    }

    // Node-specific data for K2Nodes
    if (const UK2Node *K2Node = Cast<UK2Node>(Node))
    {
        OutNodeData.bIsPure = K2Node->IsNodePure();

        // K2Node_CallFunction - serialize FunctionReference
        if (const UK2Node_CallFunction *CallFuncNode = Cast<UK2Node_CallFunction>(Node))
        {
            SerializeMemberReference(CallFuncNode->FunctionReference, OutNodeData.FunctionReference);
        }
        // K2Node_Event - serialize EventReference
        else if (const UK2Node_Event *EventNode = Cast<UK2Node_Event>(Node))
        {
            SerializeMemberReference(EventNode->EventReference, OutNodeData.EventReference);

            // Check for custom event
            if (const UK2Node_CustomEvent *CustomEventNode = Cast<UK2Node_CustomEvent>(Node))
            {
                OutNodeData.CustomEventName = CustomEventNode->CustomFunctionName.ToString();
            }
        }
        // K2Node_VariableGet/Set - serialize VariableReference
        else if (const UK2Node_VariableGet *VarGetNode = Cast<UK2Node_VariableGet>(Node))
        {
            SerializeMemberReference(VarGetNode->VariableReference, OutNodeData.VariableReference);
        }
        else if (const UK2Node_VariableSet *VarSetNode = Cast<UK2Node_VariableSet>(Node))
        {
            SerializeMemberReference(VarSetNode->VariableReference, OutNodeData.VariableReference);
        }
        // K2Node_DynamicCast - serialize TargetClass
        else if (const UK2Node_DynamicCast *CastNode = Cast<UK2Node_DynamicCast>(Node))
        {
            if (CastNode->TargetType)
            {
                OutNodeData.TargetClass = CastNode->TargetType->GetPathName();
            }
            // Use IsNodePure() instead of deprecated bIsPureCast
            // Note: We already set bIsPure above for all K2Nodes
        }
        // K2Node_SpawnActorFromClass - serialize SpawnClass
        else if (const UK2Node_SpawnActorFromClass *SpawnNode = Cast<UK2Node_SpawnActorFromClass>(Node))
        {
            if (UClass *SpawnClass = SpawnNode->GetClassToSpawn())
            {
                OutNodeData.SpawnClass = SpawnClass->GetPathName();
            }
        }
        // K2Node_ConstructObjectFromClass
        else if (const UK2Node_ConstructObjectFromClass *ConstructNode = Cast<UK2Node_ConstructObjectFromClass>(Node))
        {
            if (UClass *ClassToConstruct = ConstructNode->GetClassToSpawn())
            {
                OutNodeData.SpawnClass = ClassToConstruct->GetPathName();
            }
        }
        // K2Node_Timeline - serialize TimelineName
        else if (const UK2Node_Timeline *TimelineNode = Cast<UK2Node_Timeline>(Node))
        {
            OutNodeData.TimelineName = TimelineNode->TimelineName.ToString();
            OutNodeData.bIsLatent = true;
        }
        // K2Node_MacroInstance - serialize MacroReference
        else if (const UK2Node_MacroInstance *MacroNode = Cast<UK2Node_MacroInstance>(Node))
        {
            if (UEdGraph *MacroGraph = MacroNode->GetMacroGraph())
            {
                OutNodeData.MacroReference = MacroGraph->GetPathName();
            }
        }
        // K2Node_SwitchEnum - serialize EnumType
        else if (const UK2Node_SwitchEnum *SwitchEnumNode = Cast<UK2Node_SwitchEnum>(Node))
        {
            if (UEnum *EnumType = SwitchEnumNode->Enum)
            {
                OutNodeData.EnumType = EnumType->GetPathName();
            }
        }
        // K2Node_EnumLiteral - serialize EnumType
        else if (const UK2Node_EnumLiteral *EnumLiteralNode = Cast<UK2Node_EnumLiteral>(Node))
        {
            if (UEnum *EnumType = EnumLiteralNode->Enum)
            {
                OutNodeData.EnumType = EnumType->GetPathName();
            }
        }
        // K2Node_MakeStruct, K2Node_BreakStruct, K2Node_SetFieldsInStruct - serialize StructType
        else if (const UK2Node_MakeStruct *MakeStructNode = Cast<UK2Node_MakeStruct>(Node))
        {
            if (UScriptStruct *StructType = MakeStructNode->StructType)
            {
                OutNodeData.StructType = StructType->GetPathName();
            }
        }
        else if (const UK2Node_BreakStruct *BreakStructNode = Cast<UK2Node_BreakStruct>(Node))
        {
            if (UScriptStruct *StructType = BreakStructNode->StructType)
            {
                OutNodeData.StructType = StructType->GetPathName();
            }
        }
        else if (const UK2Node_SetFieldsInStruct *SetFieldsNode = Cast<UK2Node_SetFieldsInStruct>(Node))
        {
            if (UScriptStruct *StructType = SetFieldsNode->StructType)
            {
                OutNodeData.StructType = StructType->GetPathName();
            }
        }
        // Delegate nodes - serialize DelegateReference
        else if (const UK2Node_CreateDelegate *CreateDelegateNode = Cast<UK2Node_CreateDelegate>(Node))
        {
            OutNodeData.DelegateReference.MemberName = CreateDelegateNode->GetFunctionName().ToString();
        }
        // Input action nodes
        else if (const UK2Node_InputAction *InputActionNode = Cast<UK2Node_InputAction>(Node))
        {
            OutNodeData.InputActionName = InputActionNode->InputActionName.ToString();
        }
        else if (const UK2Node_InputKey *InputKeyNode = Cast<UK2Node_InputKey>(Node))
        {
            OutNodeData.InputKey = InputKeyNode->InputKey.GetFName().ToString();
        }
        // K2Node_Literal
        else if (const UK2Node_Literal *LiteralNode = Cast<UK2Node_Literal>(Node))
        {
            if (LiteralNode->GetObjectRef())
            {
                OutNodeData.LiteralValue = LiteralNode->GetObjectRef()->GetPathName();
            }
        }
        // K2Node_FunctionEntry
        else if (const UK2Node_FunctionEntry *FuncEntryNode = Cast<UK2Node_FunctionEntry>(Node))
        {
            // Get function name from the graph
            if (UEdGraph *Graph = FuncEntryNode->GetGraph())
            {
                // Store in NodeSpecificData since we don't have a dedicated field
                OutNodeData.NodeSpecificData.Add(TEXT("FunctionName"), Graph->GetName());
            }
        }
        // K2Node_BaseAsyncTask / K2Node_AsyncAction
        // Note: ProxyClass and ProxyFactoryClass are protected members
        // We mark these as latent but skip the class info
        else if (Cast<UK2Node_BaseAsyncTask>(Node) != nullptr)
        {
            OutNodeData.bIsLatent = true;
        }

        // Keep backward compatibility with NodeSpecificData
        OutNodeData.NodeSpecificData.Add(TEXT("IsNodePure"), K2Node->IsNodePure() ? TEXT("true") : TEXT("false"));
    }
}

void FBlueprintToJson::SerializePin(const UEdGraphPin *Pin, FBlueprintJsonPin &OutPinData)
{
    if (!Pin)
    {
        return;
    }

    OutPinData.PinId = Pin->PinId.ToString();
    OutPinData.PinName = Pin->PinName.ToString();
    OutPinData.Direction = (Pin->Direction == EGPD_Input) ? TEXT("input") : TEXT("output");
    OutPinData.PinType = PinTypeToString(Pin->PinType);
    OutPinData.DefaultValue = Pin->DefaultValue;

    // Serialize connections
    for (UEdGraphPin *LinkedPin : Pin->LinkedTo)
    {
        if (LinkedPin)
        {
            OutPinData.LinkedTo.Add(LinkedPin->PinId.ToString());
        }
    }
}

FString FBlueprintToJson::PinTypeToString(const FEdGraphPinType &PinType)
{
    FString Result;

    // Container type
    if (PinType.ContainerType == EPinContainerType::Array)
    {
        Result = TEXT("Array<");
    }
    else if (PinType.ContainerType == EPinContainerType::Set)
    {
        Result = TEXT("Set<");
    }
    else if (PinType.ContainerType == EPinContainerType::Map)
    {
        Result = TEXT("Map<");
    }

    // Base type
    Result += PinType.PinCategory.ToString();

    // Subtype for objects/structs
    if (PinType.PinSubCategoryObject.IsValid())
    {
        Result += TEXT(":") + PinType.PinSubCategoryObject->GetPathName();
    }
    else if (!PinType.PinSubCategory.IsNone())
    {
        Result += TEXT(":") + PinType.PinSubCategory.ToString();
    }

    // Close container
    if (PinType.ContainerType == EPinContainerType::Array ||
        PinType.ContainerType == EPinContainerType::Set)
    {
        Result += TEXT(">");
    }
    else if (PinType.ContainerType == EPinContainerType::Map)
    {
        // Map has value type
        Result += TEXT(",") + PinType.PinValueType.TerminalCategory.ToString();
        if (PinType.PinValueType.TerminalSubCategoryObject.IsValid())
        {
            Result += TEXT(":") + PinType.PinValueType.TerminalSubCategoryObject->GetPathName();
        }
        Result += TEXT(">");
    }

    // Reference flag
    if (PinType.bIsReference)
    {
        Result += TEXT("&");
    }

    return Result;
}

void FBlueprintToJson::SerializeMemberReference(const FMemberReference &MemberRef, FBlueprintJsonMemberReference &OutJsonRef)
{
    OutJsonRef.MemberName = MemberRef.GetMemberName().ToString();
    OutJsonRef.MemberGuid = MemberRef.GetMemberGuid().ToString();
    OutJsonRef.bIsSelfContext = MemberRef.IsSelfContext();
    OutJsonRef.bIsLocalScope = MemberRef.IsLocalScope();

    // Get parent class path if available
    if (UClass *MemberParent = MemberRef.GetMemberParentClass())
    {
        OutJsonRef.MemberParentClass = MemberParent->GetPathName();
    }

    // For function references, try to determine if it's const/pure
    if (UFunction *Func = MemberRef.ResolveMember<UFunction>(nullptr))
    {
        OutJsonRef.bIsConstFunc = (Func->FunctionFlags & FUNC_Const) != 0;
    }
}
