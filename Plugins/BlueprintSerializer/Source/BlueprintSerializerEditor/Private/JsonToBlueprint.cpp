// Copyright CattleGame. All Rights Reserved.

#include "JsonToBlueprint.h"
#include "BlueprintJsonFormat.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CustomEvent.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"

// Helper function to extract event name from node title
static FName ExtractEventNameFromTitle(const FString &NodeTitle)
{
    // NodeTitle format: "Event OnLassoThrown" or "Event OnLassoCaughtTarget"
    FString EventName = NodeTitle;
    EventName.RemoveFromStart(TEXT("Event "));
    return FName(*EventName);
}

// Helper function to find a BlueprintImplementableEvent in the parent class hierarchy
static UFunction *FindBlueprintImplementableEvent(UClass *Class, FName EventName)
{
    if (!Class)
    {
        return nullptr;
    }

    for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
    {
        UFunction *Function = *FuncIt;
        if (Function->GetFName() == EventName && Function->HasAnyFunctionFlags(FUNC_BlueprintEvent))
        {
            return Function;
        }
    }

    return nullptr;
}

UBlueprint *FJsonToBlueprint::DeserializeBlueprint(const FBlueprintJsonData &JsonData, const FString &PackagePath, const FString &BlueprintName)
{
    // Find parent class
    UClass *ParentClass = FindParentClass(JsonData);
    if (!ParentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find parent class: %s"), *JsonData.Metadata.ParentClass);
        return nullptr;
    }

    // Determine the Blueprint name
    FString FinalBlueprintName = BlueprintName.IsEmpty() ? JsonData.Metadata.BlueprintName : BlueprintName;

    // Create the Blueprint asset
    UBlueprint *Blueprint = CreateBlueprintAsset(JsonData, PackagePath, FinalBlueprintName, ParentClass);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Blueprint asset"));
        return nullptr;
    }

    // Deserialize components
    DeserializeVariables(Blueprint, JsonData);
    DeserializeComponents(Blueprint, JsonData);
    DeserializeInterfaces(Blueprint, JsonData);
    DeserializeEventGraphs(Blueprint, JsonData);
    DeserializeFunctions(Blueprint, JsonData);

    // Compile the Blueprint
    if (!CompileBlueprint(Blueprint))
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint compilation had errors"));
    }

    return Blueprint;
}

UBlueprint *FJsonToBlueprint::DeserializeBlueprintFromString(const FString &JsonString, const FString &PackagePath, const FString &BlueprintName)
{
    FBlueprintJsonData JsonData;
    if (!ParseJsonString(JsonString, JsonData))
    {
        return nullptr;
    }

    return DeserializeBlueprint(JsonData, PackagePath, BlueprintName);
}

UBlueprint *FJsonToBlueprint::DeserializeBlueprintFromFile(const FString &FilePath, const FString &PackagePath, const FString &BlueprintName)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *FilePath);
        return nullptr;
    }

    return DeserializeBlueprintFromString(JsonString, PackagePath, BlueprintName);
}

bool FJsonToBlueprint::ParseJsonString(const FString &JsonString, FBlueprintJsonData &OutJsonData)
{
    return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutJsonData, 0, 0);
}

bool FJsonToBlueprint::MergeJsonIntoBlueprint(UBlueprint *TargetBlueprint, const FBlueprintJsonData &JsonData)
{
    if (!TargetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("MergeJsonIntoBlueprint: Target Blueprint is null"));
        return false;
    }

    // Merge variables (add new ones, don't remove existing)
    MergeVariables(TargetBlueprint, JsonData);

    // Merge event graph nodes (add new nodes)
    MergeEventGraphs(TargetBlueprint, JsonData);

    // Compile the Blueprint
    if (!CompileBlueprint(TargetBlueprint))
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint compilation had errors after merge"));
    }

    // Mark as modified
    TargetBlueprint->MarkPackageDirty();

    return true;
}

bool FJsonToBlueprint::MergeJsonFileIntoBlueprint(UBlueprint *TargetBlueprint, const FString &FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *FilePath);
        return false;
    }

    FBlueprintJsonData JsonData;
    if (!ParseJsonString(JsonString, JsonData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON file: %s"), *FilePath);
        return false;
    }

    return MergeJsonIntoBlueprint(TargetBlueprint, JsonData);
}

UClass *FJsonToBlueprint::FindParentClass(const FBlueprintJsonData &JsonData)
{
    if (JsonData.Metadata.ParentClass.IsEmpty())
    {
        return AActor::StaticClass(); // Default to Actor
    }

    // Try to find the class
    UClass *ParentClass = FindObject<UClass>(nullptr, *JsonData.Metadata.ParentClass);
    if (!ParentClass)
    {
        // Try loading it
        ParentClass = LoadClass<UObject>(nullptr, *JsonData.Metadata.ParentClass);
    }

    return ParentClass ? ParentClass : AActor::StaticClass();
}

UBlueprint *FJsonToBlueprint::CreateBlueprintAsset(const FBlueprintJsonData &JsonData, const FString &PackagePath, const FString &BlueprintName, UClass *ParentClass)
{
    // Create package
    FString FullPackagePath = PackagePath / BlueprintName;
    UPackage *Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        return nullptr;
    }

    // Determine Blueprint type
    EBlueprintType BlueprintType = BPTYPE_Normal;
    if (JsonData.Metadata.BlueprintType == TEXT("Const"))
    {
        BlueprintType = BPTYPE_Const;
    }
    else if (JsonData.Metadata.BlueprintType == TEXT("MacroLibrary"))
    {
        BlueprintType = BPTYPE_MacroLibrary;
    }
    else if (JsonData.Metadata.BlueprintType == TEXT("Interface"))
    {
        BlueprintType = BPTYPE_Interface;
    }
    else if (JsonData.Metadata.BlueprintType == TEXT("FunctionLibrary"))
    {
        BlueprintType = BPTYPE_FunctionLibrary;
    }

    // Create the Blueprint
    UBlueprint *Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        *BlueprintName,
        BlueprintType,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass());

    if (Blueprint)
    {
        // Mark package as dirty
        Package->MarkPackageDirty();

        // Register with asset registry
        FAssetRegistryModule::AssetCreated(Blueprint);
    }

    return Blueprint;
}

void FJsonToBlueprint::DeserializeVariables(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    for (const FBlueprintJsonVariable &JsonVar : JsonData.Variables)
    {
        FEdGraphPinType PinType;
        if (!StringToPinType(JsonVar.VarType, PinType))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to parse variable type: %s for variable %s"), *JsonVar.VarType, *JsonVar.VarName);
            continue;
        }

        // Add the variable
        FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*JsonVar.VarName), PinType, JsonVar.DefaultValue);

        // Find the variable we just added and set additional properties
        for (FBPVariableDescription &VarDesc : Blueprint->NewVariables)
        {
            if (VarDesc.VarName == FName(*JsonVar.VarName))
            {
                // Category
                if (!JsonVar.Category.IsEmpty())
                {
                    VarDesc.Category = FText::FromString(JsonVar.Category);
                }

                // Exposed
                if (JsonVar.bIsExposed)
                {
                    VarDesc.PropertyFlags |= CPF_ExposeOnSpawn;
                }

                // Read only
                if (JsonVar.bIsReadOnly)
                {
                    VarDesc.PropertyFlags |= CPF_BlueprintReadOnly;
                }

                // Replication
                if (JsonVar.ReplicationCondition == TEXT("Replicated"))
                {
                    VarDesc.PropertyFlags |= CPF_Net;
                }
                else if (JsonVar.ReplicationCondition == TEXT("RepNotify"))
                {
                    VarDesc.PropertyFlags |= CPF_RepNotify;
                }

                // Metadata
                for (const auto &MetaPair : JsonVar.Metadata)
                {
                    FBPVariableMetaDataEntry MetaEntry;
                    MetaEntry.DataKey = FName(*MetaPair.Key);
                    MetaEntry.DataValue = MetaPair.Value;
                    VarDesc.MetaDataArray.Add(MetaEntry);
                }

                break;
            }
        }
    }
}

void FJsonToBlueprint::MergeVariables(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    // Build a set of existing variable names
    TSet<FName> ExistingVarNames;
    for (const FBPVariableDescription &VarDesc : Blueprint->NewVariables)
    {
        ExistingVarNames.Add(VarDesc.VarName);
    }

    // Only add variables that don't already exist
    for (const FBlueprintJsonVariable &JsonVar : JsonData.Variables)
    {
        FName VarName(*JsonVar.VarName);
        if (ExistingVarNames.Contains(VarName))
        {
            UE_LOG(LogTemp, Log, TEXT("MergeVariables: Skipping existing variable: %s"), *JsonVar.VarName);
            continue;
        }

        FEdGraphPinType PinType;
        if (!StringToPinType(JsonVar.VarType, PinType))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to parse variable type: %s for variable %s"), *JsonVar.VarType, *JsonVar.VarName);
            continue;
        }

        // Add the variable
        FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarName, PinType, JsonVar.DefaultValue);
        UE_LOG(LogTemp, Log, TEXT("MergeVariables: Added new variable: %s"), *JsonVar.VarName);

        // Find the variable we just added and set additional properties
        for (FBPVariableDescription &VarDesc : Blueprint->NewVariables)
        {
            if (VarDesc.VarName == VarName)
            {
                if (!JsonVar.Category.IsEmpty())
                {
                    VarDesc.Category = FText::FromString(JsonVar.Category);
                }
                if (JsonVar.bIsExposed)
                {
                    VarDesc.PropertyFlags |= CPF_ExposeOnSpawn;
                }
                if (JsonVar.bIsReadOnly)
                {
                    VarDesc.PropertyFlags |= CPF_BlueprintReadOnly;
                }
                break;
            }
        }
    }
}

void FJsonToBlueprint::MergeEventGraphs(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    // Get the parent class to find BlueprintImplementableEvents
    UClass *ParentClass = Blueprint->ParentClass;

    for (const FBlueprintJsonGraph &JsonGraph : JsonData.EventGraphs)
    {
        // Find the event graph
        UEdGraph *Graph = nullptr;
        for (UEdGraph *ExistingGraph : Blueprint->UbergraphPages)
        {
            if (ExistingGraph && ExistingGraph->GetName() == JsonGraph.GraphName)
            {
                Graph = ExistingGraph;
                break;
            }
        }

        if (!Graph && Blueprint->UbergraphPages.Num() > 0)
        {
            Graph = Blueprint->UbergraphPages[0];
        }

        if (!Graph)
        {
            UE_LOG(LogTemp, Warning, TEXT("MergeEventGraphs: Could not find event graph: %s"), *JsonGraph.GraphName);
            continue;
        }

        // Build a set of existing node GUIDs
        TSet<FString> ExistingNodeGuids;
        for (UEdGraphNode *Node : Graph->Nodes)
        {
            if (Node)
            {
                ExistingNodeGuids.Add(Node->NodeGuid.ToString());
            }
        }

        // Build a set of existing event function names
        TSet<FName> ExistingEventFunctions;
        for (UEdGraphNode *Node : Graph->Nodes)
        {
            if (UK2Node_Event *EventNode = Cast<UK2Node_Event>(Node))
            {
                if (EventNode->EventReference.GetMemberName() != NAME_None)
                {
                    ExistingEventFunctions.Add(EventNode->EventReference.GetMemberName());
                }
                // Also check custom event names
                if (EventNode->CustomFunctionName != NAME_None)
                {
                    ExistingEventFunctions.Add(EventNode->CustomFunctionName);
                }
            }
        }

        // Add new nodes
        int32 NodesAdded = 0;
        TMap<FString, UEdGraphNode *> NewNodeMap;

        for (const FBlueprintJsonNode &JsonNode : JsonGraph.Nodes)
        {
            // Skip if this exact node already exists (by GUID)
            if (ExistingNodeGuids.Contains(JsonNode.NodeGuid))
            {
                UE_LOG(LogTemp, Log, TEXT("MergeEventGraphs: Skipping existing node GUID: %s"), *JsonNode.NodeGuid);
                continue;
            }

            // Special handling for event nodes - create proper event overrides
            if (JsonNode.NodeClass == TEXT("K2Node_Event"))
            {
                FName EventName = ExtractEventNameFromTitle(JsonNode.NodeTitle);

                // Skip if this event already exists
                if (ExistingEventFunctions.Contains(EventName))
                {
                    UE_LOG(LogTemp, Log, TEXT("MergeEventGraphs: Skipping existing event: %s"), *EventName.ToString());
                    continue;
                }

                // Find the BlueprintImplementableEvent in parent class
                UFunction *EventFunction = FindBlueprintImplementableEvent(ParentClass, EventName);
                if (EventFunction)
                {
                    // Create the event node using the proper method
                    int32 NodeIndex = INDEX_NONE;
                    UK2Node_Event *EventNode = FKismetEditorUtilities::AddDefaultEventNode(
                        Blueprint,
                        Graph,
                        EventName,
                        ParentClass,
                        NodeIndex);

                    if (EventNode)
                    {
                        // Set position
                        EventNode->NodePosX = JsonNode.PositionX;
                        EventNode->NodePosY = JsonNode.PositionY;
                        EventNode->NodeComment = JsonNode.NodeComment;

                        NewNodeMap.Add(JsonNode.NodeGuid, EventNode);
                        NodesAdded++;
                        UE_LOG(LogTemp, Log, TEXT("MergeEventGraphs: Added event override: %s"), *EventName.ToString());
                        continue;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("MergeEventGraphs: Failed to create event node for: %s"), *EventName.ToString());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("MergeEventGraphs: Could not find BlueprintImplementableEvent: %s in class %s"),
                           *EventName.ToString(), *ParentClass->GetName());
                }

                // If we couldn't create a proper event override, skip this node
                continue;
            }

            // For non-event nodes, use the regular deserialization
            UEdGraphNode *NewNode = DeserializeNode(Graph, JsonNode);
            if (NewNode)
            {
                NewNodeMap.Add(JsonNode.NodeGuid, NewNode);
                NodesAdded++;
                UE_LOG(LogTemp, Log, TEXT("MergeEventGraphs: Added node: %s (%s)"), *JsonNode.NodeTitle, *JsonNode.NodeClass);
            }
        }

        // Link pins for newly added nodes only
        if (NodesAdded > 0)
        {
            // Build pin map for new nodes
            TMap<FString, UEdGraphPin *> NewPinMap;
            for (const auto &Pair : NewNodeMap)
            {
                UEdGraphNode *Node = Pair.Value;
                if (Node)
                {
                    for (UEdGraphPin *Pin : Node->Pins)
                    {
                        if (Pin)
                        {
                            NewPinMap.Add(Pin->PinId.ToString(), Pin);
                        }
                    }
                }
            }

            // Link pins between new nodes
            for (const FBlueprintJsonNode &JsonNode : JsonGraph.Nodes)
            {
                UEdGraphNode **NewNodePtr = NewNodeMap.Find(JsonNode.NodeGuid);
                if (!NewNodePtr)
                {
                    continue;
                }

                for (const FBlueprintJsonPin &JsonPin : JsonNode.Pins)
                {
                    UEdGraphPin **SourcePinPtr = NewPinMap.Find(JsonPin.PinId);
                    if (!SourcePinPtr)
                    {
                        continue;
                    }

                    for (const FString &LinkedPinId : JsonPin.LinkedTo)
                    {
                        UEdGraphPin **TargetPinPtr = NewPinMap.Find(LinkedPinId);
                        if (TargetPinPtr)
                        {
                            (*SourcePinPtr)->MakeLinkTo(*TargetPinPtr);
                        }
                    }
                }
            }

            UE_LOG(LogTemp, Log, TEXT("MergeEventGraphs: Added %d new nodes to graph %s"), NodesAdded, *JsonGraph.GraphName);
        }
    }
}

void FJsonToBlueprint::DeserializeEventGraphs(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    for (const FBlueprintJsonGraph &JsonGraph : JsonData.EventGraphs)
    {
        // Find or create the event graph
        UEdGraph *Graph = nullptr;
        for (UEdGraph *ExistingGraph : Blueprint->UbergraphPages)
        {
            if (ExistingGraph && ExistingGraph->GetName() == JsonGraph.GraphName)
            {
                Graph = ExistingGraph;
                break;
            }
        }

        if (!Graph && Blueprint->UbergraphPages.Num() > 0)
        {
            // Use the default event graph
            Graph = Blueprint->UbergraphPages[0];
        }

        if (Graph)
        {
            DeserializeGraph(Graph, JsonGraph);
            LinkPins(Graph, JsonGraph);
        }
    }
}

void FJsonToBlueprint::DeserializeFunctions(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    for (const FBlueprintJsonFunction &JsonFunc : JsonData.Functions)
    {
        // Create function graph
        UEdGraph *FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            FName(*JsonFunc.FunctionName),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());

        if (FunctionGraph)
        {
            FBlueprintEditorUtils::AddFunctionGraph<UFunction>(Blueprint, FunctionGraph, true, static_cast<UFunction *>(nullptr));
            DeserializeGraph(FunctionGraph, JsonFunc.Graph);
            LinkPins(FunctionGraph, JsonFunc.Graph);
        }
    }
}

void FJsonToBlueprint::DeserializeComponents(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    if (!Blueprint->SimpleConstructionScript)
    {
        return;
    }

    for (const FBlueprintJsonComponent &JsonComp : JsonData.Components)
    {
        // Find the component class
        UClass *ComponentClass = FindObject<UClass>(nullptr, *JsonComp.ComponentClass);
        if (!ComponentClass)
        {
            ComponentClass = LoadClass<UActorComponent>(nullptr, *JsonComp.ComponentClass);
        }

        if (!ComponentClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to find component class: %s"), *JsonComp.ComponentClass);
            continue;
        }

        // Create the SCS node
        USCS_Node *NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, FName(*JsonComp.ComponentName));
        if (NewNode)
        {
            // Add to the tree
            if (JsonComp.ParentComponent.IsEmpty())
            {
                Blueprint->SimpleConstructionScript->AddNode(NewNode);
            }
            else
            {
                // Find parent node and attach
                for (USCS_Node *ExistingNode : Blueprint->SimpleConstructionScript->GetAllNodes())
                {
                    if (ExistingNode && ExistingNode->GetVariableName().ToString() == JsonComp.ParentComponent)
                    {
                        ExistingNode->AddChildNode(NewNode);
                        break;
                    }
                }
            }

            // TODO: Apply component properties from JsonComp.Properties
        }
    }
}

void FJsonToBlueprint::DeserializeInterfaces(UBlueprint *Blueprint, const FBlueprintJsonData &JsonData)
{
    for (const FString &InterfacePath : JsonData.ImplementedInterfaces)
    {
        UClass *InterfaceClass = FindObject<UClass>(nullptr, *InterfacePath);
        if (!InterfaceClass)
        {
            InterfaceClass = LoadClass<UInterface>(nullptr, *InterfacePath);
        }

        if (InterfaceClass)
        {
            FTopLevelAssetPath InterfaceAssetPath(InterfaceClass->GetPathName());
            FBlueprintEditorUtils::ImplementNewInterface(Blueprint, InterfaceAssetPath);
        }
    }
}

void FJsonToBlueprint::DeserializeGraph(UEdGraph *Graph, const FBlueprintJsonGraph &GraphData)
{
    if (!Graph)
    {
        return;
    }

    // Create nodes
    for (const FBlueprintJsonNode &JsonNode : GraphData.Nodes)
    {
        DeserializeNode(Graph, JsonNode);
    }
}

UEdGraphNode *FJsonToBlueprint::DeserializeNode(UEdGraph *Graph, const FBlueprintJsonNode &NodeData)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Find the node class
    UClass *NodeClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/BlueprintGraph.%s"), *NodeData.NodeClass));
    if (!NodeClass)
    {
        // Try just the class name in various modules
        NodeClass = FindFirstObjectSafe<UClass>(*NodeData.NodeClass);
    }

    if (!NodeClass || !NodeClass->IsChildOf(UEdGraphNode::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find node class: %s"), *NodeData.NodeClass);
        return nullptr;
    }

    // Special handling for K2Node_CallFunction - need to set function before creating pins
    if (NodeData.NodeClass == TEXT("K2Node_CallFunction"))
    {
        UK2Node_CallFunction *CallNode = NewObject<UK2Node_CallFunction>(Graph);
        if (!CallNode)
        {
            return nullptr;
        }

        // Set position and comment
        CallNode->NodePosX = NodeData.PositionX;
        CallNode->NodePosY = NodeData.PositionY;
        CallNode->NodeComment = NodeData.NodeComment;

        // Parse GUID
        if (!NodeData.NodeGuid.IsEmpty())
        {
            FGuid ParsedGuid;
            if (FGuid::Parse(NodeData.NodeGuid, ParsedGuid))
            {
                CallNode->NodeGuid = ParsedGuid;
            }
        }

        // Use FunctionReference to set up the node
        UFunction *Function = ResolveMemberReferenceAsFunction(NodeData.FunctionReference);
        if (Function)
        {
            CallNode->SetFromFunction(Function);
            UE_LOG(LogTemp, Log, TEXT("DeserializeNode: Set CallFunction to %s::%s"),
                   Function->GetOuter() ? *Function->GetOuter()->GetName() : TEXT("None"),
                   *Function->GetName());
        }
        else if (!NodeData.FunctionReference.MemberName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("DeserializeNode: Could not resolve function: %s in %s"),
                   *NodeData.FunctionReference.MemberName, *NodeData.FunctionReference.MemberParentClass);

            // Fallback: try the old NodeSpecificData approach
            const FString *FunctionRefStr = NodeData.NodeSpecificData.Find(TEXT("FunctionReference"));
            if (FunctionRefStr && !FunctionRefStr->IsEmpty())
            {
                Function = ResolveFunctionFromPath(*FunctionRefStr);
                if (Function)
                {
                    CallNode->SetFromFunction(Function);
                }
            }
        }

        // Add to graph and create pins
        Graph->AddNode(CallNode, false, false);
        CallNode->CreateNewGuid();
        CallNode->PostPlacedNewNode();
        CallNode->AllocateDefaultPins();

        return CallNode;
    }

    // Special handling for K2Node_VariableGet
    if (NodeData.NodeClass == TEXT("K2Node_VariableGet"))
    {
        UK2Node_VariableGet *VarGetNode = NewObject<UK2Node_VariableGet>(Graph);
        if (!VarGetNode)
        {
            return nullptr;
        }

        // Set position and comment
        VarGetNode->NodePosX = NodeData.PositionX;
        VarGetNode->NodePosY = NodeData.PositionY;
        VarGetNode->NodeComment = NodeData.NodeComment;

        // Set variable reference
        if (!NodeData.VariableReference.MemberName.IsEmpty())
        {
            ApplyMemberReferenceToVariable(NodeData.VariableReference, VarGetNode->VariableReference, Graph);
        }

        // Add to graph and create pins
        Graph->AddNode(VarGetNode, false, false);
        VarGetNode->CreateNewGuid();
        VarGetNode->PostPlacedNewNode();
        VarGetNode->AllocateDefaultPins();

        return VarGetNode;
    }

    // Special handling for K2Node_VariableSet
    if (NodeData.NodeClass == TEXT("K2Node_VariableSet"))
    {
        UK2Node_VariableSet *VarSetNode = NewObject<UK2Node_VariableSet>(Graph);
        if (!VarSetNode)
        {
            return nullptr;
        }

        // Set position and comment
        VarSetNode->NodePosX = NodeData.PositionX;
        VarSetNode->NodePosY = NodeData.PositionY;
        VarSetNode->NodeComment = NodeData.NodeComment;

        // Set variable reference
        if (!NodeData.VariableReference.MemberName.IsEmpty())
        {
            ApplyMemberReferenceToVariable(NodeData.VariableReference, VarSetNode->VariableReference, Graph);
        }

        // Add to graph and create pins
        Graph->AddNode(VarSetNode, false, false);
        VarSetNode->CreateNewGuid();
        VarSetNode->PostPlacedNewNode();
        VarSetNode->AllocateDefaultPins();

        return VarSetNode;
    }

    // Create the node generically for other types
    UEdGraphNode *NewNode = NewObject<UEdGraphNode>(Graph, NodeClass);
    if (!NewNode)
    {
        return nullptr;
    }

    // Set basic properties
    NewNode->NodePosX = NodeData.PositionX;
    NewNode->NodePosY = NodeData.PositionY;
    NewNode->NodeComment = NodeData.NodeComment;

    // Parse and set GUID if provided
    if (!NodeData.NodeGuid.IsEmpty())
    {
        FGuid ParsedGuid;
        if (FGuid::Parse(NodeData.NodeGuid, ParsedGuid))
        {
            NewNode->NodeGuid = ParsedGuid;
        }
    }

    // Add to graph
    Graph->AddNode(NewNode, false, false);
    NewNode->CreateNewGuid();
    NewNode->PostPlacedNewNode();
    NewNode->AllocateDefaultPins();

    return NewNode;
}

void FJsonToBlueprint::LinkPins(UEdGraph *Graph, const FBlueprintJsonGraph &GraphData)
{
    if (!Graph)
    {
        return;
    }

    // Build a map of PinId -> UEdGraphPin*
    TMap<FString, UEdGraphPin *> PinMap;
    for (UEdGraphNode *Node : Graph->Nodes)
    {
        if (Node)
        {
            for (UEdGraphPin *Pin : Node->Pins)
            {
                if (Pin)
                {
                    PinMap.Add(Pin->PinId.ToString(), Pin);
                }
            }
        }
    }

    // Link pins based on JSON data
    for (const FBlueprintJsonNode &JsonNode : GraphData.Nodes)
    {
        for (const FBlueprintJsonPin &JsonPin : JsonNode.Pins)
        {
            UEdGraphPin **SourcePinPtr = PinMap.Find(JsonPin.PinId);
            if (!SourcePinPtr)
            {
                continue;
            }

            UEdGraphPin *SourcePin = *SourcePinPtr;
            for (const FString &LinkedPinId : JsonPin.LinkedTo)
            {
                UEdGraphPin **TargetPinPtr = PinMap.Find(LinkedPinId);
                if (TargetPinPtr)
                {
                    SourcePin->MakeLinkTo(*TargetPinPtr);
                }
            }
        }
    }
}

bool FJsonToBlueprint::StringToPinType(const FString &TypeString, FEdGraphPinType &OutPinType)
{
    // Parse the type string
    // Format: [Container<]Category[:SubCategory][>][&]

    FString WorkString = TypeString;

    // Check for reference
    if (WorkString.EndsWith(TEXT("&")))
    {
        OutPinType.bIsReference = true;
        WorkString = WorkString.LeftChop(1);
    }

    // Check for container type
    if (WorkString.StartsWith(TEXT("Array<")))
    {
        OutPinType.ContainerType = EPinContainerType::Array;
        WorkString = WorkString.Mid(6);
        WorkString = WorkString.LeftChop(1); // Remove closing >
    }
    else if (WorkString.StartsWith(TEXT("Set<")))
    {
        OutPinType.ContainerType = EPinContainerType::Set;
        WorkString = WorkString.Mid(4);
        WorkString = WorkString.LeftChop(1);
    }
    else if (WorkString.StartsWith(TEXT("Map<")))
    {
        OutPinType.ContainerType = EPinContainerType::Map;
        WorkString = WorkString.Mid(4);
        WorkString = WorkString.LeftChop(1);

        // TODO: Parse map value type
    }

    // Parse category and subcategory
    int32 ColonIndex;
    if (WorkString.FindChar(TEXT(':'), ColonIndex))
    {
        OutPinType.PinCategory = FName(*WorkString.Left(ColonIndex));
        FString SubCategory = WorkString.Mid(ColonIndex + 1);

        // Try to find the subcategory object
        UObject *SubCategoryObject = FindObject<UObject>(nullptr, *SubCategory);
        if (!SubCategoryObject)
        {
            SubCategoryObject = LoadObject<UObject>(nullptr, *SubCategory);
        }

        if (SubCategoryObject)
        {
            OutPinType.PinSubCategoryObject = SubCategoryObject;
        }
        else
        {
            OutPinType.PinSubCategory = FName(*SubCategory);
        }
    }
    else
    {
        OutPinType.PinCategory = FName(*WorkString);
    }

    return true;
}

bool FJsonToBlueprint::CompileBlueprint(UBlueprint *Blueprint)
{
    if (!Blueprint)
    {
        return false;
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection);

    return Blueprint->Status != BS_Error;
}

UFunction *FJsonToBlueprint::ResolveMemberReferenceAsFunction(const FBlueprintJsonMemberReference &MemberRef)
{
    if (MemberRef.MemberName.IsEmpty())
    {
        return nullptr;
    }

    // If we have a parent class, use it to find the function
    if (!MemberRef.MemberParentClass.IsEmpty())
    {
        UClass *ParentClass = FindParentClassFromPath(MemberRef.MemberParentClass);
        if (ParentClass)
        {
            UFunction *Func = ParentClass->FindFunctionByName(*MemberRef.MemberName);
            if (Func)
            {
                return Func;
            }
        }
    }

    // Fallback: search all loaded classes for the function
    // This is slower but more reliable for edge cases
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass *Class = *ClassIt;
        UFunction *Func = Class->FindFunctionByName(*MemberRef.MemberName, EIncludeSuperFlag::ExcludeSuper);
        if (Func)
        {
            return Func;
        }
    }

    return nullptr;
}

UClass *FJsonToBlueprint::FindParentClassFromPath(const FString &ClassPath)
{
    if (ClassPath.IsEmpty())
    {
        return nullptr;
    }

    // Direct path lookup
    UClass *FoundClass = FindObject<UClass>(nullptr, *ClassPath);
    if (FoundClass)
    {
        return FoundClass;
    }

    // Try loading the class
    FoundClass = LoadObject<UClass>(nullptr, *ClassPath);
    if (FoundClass)
    {
        return FoundClass;
    }

    // Parse the path to extract class name and try FindFirstObjectSafe
    // Path format: /Script/ModuleName.ClassName
    if (ClassPath.StartsWith(TEXT("/Script/")))
    {
        FString PathWithoutScript = ClassPath.RightChop(8); // Remove "/Script/"

        int32 DotIndex;
        if (PathWithoutScript.FindChar(TEXT('.'), DotIndex))
        {
            FString ClassName = PathWithoutScript.RightChop(DotIndex + 1);
            FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
            if (FoundClass)
            {
                return FoundClass;
            }

            // Try with U prefix for UObject classes
            FoundClass = FindFirstObjectSafe<UClass>(*FString::Printf(TEXT("U%s"), *ClassName));
            if (FoundClass)
            {
                return FoundClass;
            }
        }
    }

    return nullptr;
}

UFunction *FJsonToBlueprint::ResolveFunctionFromPath(const FString &FunctionPath)
{
    if (FunctionPath.IsEmpty())
    {
        return nullptr;
    }

    // Path format: /Script/ModuleName.ClassName.FunctionName
    if (FunctionPath.StartsWith(TEXT("/Script/")))
    {
        FString PathWithoutScript = FunctionPath.RightChop(8); // Remove "/Script/"

        // Find the last dot to separate function name
        int32 LastDotIndex;
        if (PathWithoutScript.FindLastChar(TEXT('.'), LastDotIndex))
        {
            FString ClassPath = FString::Printf(TEXT("/Script/%s"), *PathWithoutScript.Left(LastDotIndex));
            FString FunctionName = PathWithoutScript.RightChop(LastDotIndex + 1);

            UClass *FunctionClass = FindParentClassFromPath(ClassPath);
            if (FunctionClass)
            {
                return FunctionClass->FindFunctionByName(*FunctionName);
            }
        }
    }

    return nullptr;
}

void FJsonToBlueprint::ApplyMemberReferenceToVariable(const FBlueprintJsonMemberReference &MemberRef, FMemberReference &OutMemberRef, UEdGraph *Graph)
{
    if (MemberRef.MemberName.IsEmpty())
    {
        return;
    }

    FName MemberName = FName(*MemberRef.MemberName);
    FGuid ParsedGuid;

    // Parse GUID if available
    if (!MemberRef.MemberGuid.IsEmpty())
    {
        FGuid::Parse(MemberRef.MemberGuid, ParsedGuid);
    }

    // Determine if this is self-context or external
    if (MemberRef.bIsSelfContext)
    {
        // Self-context variable
        if (ParsedGuid.IsValid())
        {
            OutMemberRef.SetSelfMember(MemberName, ParsedGuid);
        }
        else
        {
            OutMemberRef.SetSelfMember(MemberName);
        }
    }
    else if (MemberRef.bIsLocalScope && Graph)
    {
        // Local scope variable
        UStruct *Scope = nullptr;

        // Try to get the scope from the graph's outer (the Blueprint or Function)
        if (UBlueprint *Blueprint = Cast<UBlueprint>(Graph->GetOuter()))
        {
            Scope = Blueprint->GeneratedClass;
        }

        if (Scope)
        {
            OutMemberRef.SetLocalMember(MemberName, Scope, ParsedGuid);
        }
        else
        {
            // Fallback to string-based scope
            OutMemberRef.SetLocalMember(MemberName, Graph->GetName(), ParsedGuid);
        }
    }
    else if (!MemberRef.MemberParentClass.IsEmpty())
    {
        // External variable with parent class
        UClass *ParentClass = FindParentClassFromPath(MemberRef.MemberParentClass);
        if (ParentClass)
        {
            if (ParsedGuid.IsValid())
            {
                OutMemberRef.SetExternalMember(MemberName, ParentClass, ParsedGuid);
            }
            else
            {
                OutMemberRef.SetExternalMember(MemberName, ParentClass);
            }
        }
        else
        {
            // Couldn't find parent class, use self context as fallback
            OutMemberRef.SetSelfMember(MemberName);
        }
    }
    else
    {
        // Default to self context
        OutMemberRef.SetSelfMember(MemberName);
    }
}
