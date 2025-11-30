"""
Blueprint JSON Schema Module for CattleGame

This module provides Python utilities for working with Unreal Engine Blueprint JSON
serialization format. It includes:
- Type definitions for Blueprint JSON structures
- Validation utilities
- Helper functions for creating and modifying Blueprint JSON
- Schema documentation

Compatible with BlueprintSerializer plugin version 2.0.0
"""

from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Literal
from enum import Enum
import json
import uuid


# =============================================================================
# Member Reference Types
# =============================================================================

@dataclass
class MemberReference:
    """
    Reference to a Blueprint member (function, variable, event, or delegate).
    
    This corresponds to Unreal's FMemberReference and is used for:
    - Function calls (K2Node_CallFunction.FunctionReference)
    - Events (K2Node_Event.EventReference)
    - Variables (K2Node_VariableGet/Set.VariableReference)
    - Delegates (K2Node_*Delegate.DelegateReference)
    """
    member_name: str = ""
    member_parent_class: str = ""
    member_guid: str = ""
    is_self_context: bool = False
    is_const_func: bool = False
    is_local_scope: bool = False

    def to_dict(self) -> Dict[str, Any]:
        return {
            "memberName": self.member_name,
            "memberParentClass": self.member_parent_class,
            "memberGuid": self.member_guid,
            "bIsSelfContext": self.is_self_context,
            "bIsConstFunc": self.is_const_func,
            "bIsLocalScope": self.is_local_scope
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "MemberReference":
        return cls(
            member_name=data.get("memberName", ""),
            member_parent_class=data.get("memberParentClass", ""),
            member_guid=data.get("memberGuid", ""),
            is_self_context=data.get("bIsSelfContext", False),
            is_const_func=data.get("bIsConstFunc", False),
            is_local_scope=data.get("bIsLocalScope", False)
        )

    @classmethod
    def create_self_function(cls, function_name: str) -> "MemberReference":
        """Create a reference to a function on self."""
        return cls(
            member_name=function_name,
            is_self_context=True
        )

    @classmethod
    def create_external_function(cls, class_path: str, function_name: str) -> "MemberReference":
        """Create a reference to a function on an external class."""
        return cls(
            member_name=function_name,
            member_parent_class=class_path,
            is_self_context=False
        )

    @classmethod
    def create_kismet_library_function(cls, function_name: str) -> "MemberReference":
        """Create a reference to a common Kismet System Library function."""
        return cls(
            member_name=function_name,
            member_parent_class="/Script/Engine.KismetSystemLibrary",
            is_self_context=False
        )


# =============================================================================
# Pin Types
# =============================================================================

class PinDirection(str, Enum):
    INPUT = "input"
    OUTPUT = "output"


@dataclass
class Pin:
    """
    Represents a pin on a Blueprint node.
    """
    pin_id: str = ""
    pin_name: str = ""
    direction: PinDirection = PinDirection.INPUT
    pin_type: str = ""
    default_value: str = ""
    linked_to: List[str] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "pinId": self.pin_id,
            "pinName": self.pin_name,
            "direction": self.direction.value,
            "pinType": self.pin_type,
            "defaultValue": self.default_value,
            "linkedTo": self.linked_to
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Pin":
        return cls(
            pin_id=data.get("pinId", ""),
            pin_name=data.get("pinName", ""),
            direction=PinDirection(data.get("direction", "input")),
            pin_type=data.get("pinType", ""),
            default_value=data.get("defaultValue", ""),
            linked_to=data.get("linkedTo", [])
        )

    @classmethod
    def create_exec_input(cls) -> "Pin":
        """Create an execution input pin."""
        return cls(
            pin_id=str(uuid.uuid4()),
            pin_name="execute",
            direction=PinDirection.INPUT,
            pin_type="exec"
        )

    @classmethod
    def create_exec_output(cls, name: str = "then") -> "Pin":
        """Create an execution output pin."""
        return cls(
            pin_id=str(uuid.uuid4()),
            pin_name=name,
            direction=PinDirection.OUTPUT,
            pin_type="exec"
        )


# =============================================================================
# Node Types
# =============================================================================

@dataclass
class Node:
    """
    Represents a Blueprint graph node.
    """
    node_guid: str = ""
    node_class: str = ""
    node_title: str = ""
    node_comment: str = ""
    position_x: float = 0.0
    position_y: float = 0.0
    pins: List[Pin] = field(default_factory=list)
    
    # Node-type-specific references
    function_reference: MemberReference = field(default_factory=MemberReference)
    event_reference: MemberReference = field(default_factory=MemberReference)
    variable_reference: MemberReference = field(default_factory=MemberReference)
    delegate_reference: MemberReference = field(default_factory=MemberReference)
    
    # Type references
    target_class: str = ""
    spawn_class: str = ""
    enum_type: str = ""
    struct_type: str = ""
    
    # Special node data
    macro_reference: str = ""
    timeline_name: str = ""
    literal_value: str = ""
    custom_event_name: str = ""
    input_action_name: str = ""
    input_key: str = ""
    
    # Flags
    is_pure: bool = False
    is_latent: bool = False
    self_context: bool = False
    
    # Legacy/extensibility
    node_specific_data: Dict[str, str] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        result = {
            "nodeGuid": self.node_guid,
            "nodeClass": self.node_class,
            "nodeTitle": self.node_title,
            "nodeComment": self.node_comment,
            "positionX": self.position_x,
            "positionY": self.position_y,
            "pins": [p.to_dict() for p in self.pins],
            "functionReference": self.function_reference.to_dict(),
            "eventReference": self.event_reference.to_dict(),
            "variableReference": self.variable_reference.to_dict(),
            "delegateReference": self.delegate_reference.to_dict(),
            "targetClass": self.target_class,
            "spawnClass": self.spawn_class,
            "enumType": self.enum_type,
            "structType": self.struct_type,
            "macroReference": self.macro_reference,
            "timelineName": self.timeline_name,
            "literalValue": self.literal_value,
            "customEventName": self.custom_event_name,
            "inputActionName": self.input_action_name,
            "inputKey": self.input_key,
            "bIsPure": self.is_pure,
            "bIsLatent": self.is_latent,
            "bSelfContext": self.self_context,
            "nodeSpecificData": self.node_specific_data
        }
        return result

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Node":
        return cls(
            node_guid=data.get("nodeGuid", ""),
            node_class=data.get("nodeClass", ""),
            node_title=data.get("nodeTitle", ""),
            node_comment=data.get("nodeComment", ""),
            position_x=data.get("positionX", 0.0),
            position_y=data.get("positionY", 0.0),
            pins=[Pin.from_dict(p) for p in data.get("pins", [])],
            function_reference=MemberReference.from_dict(data.get("functionReference", {})),
            event_reference=MemberReference.from_dict(data.get("eventReference", {})),
            variable_reference=MemberReference.from_dict(data.get("variableReference", {})),
            delegate_reference=MemberReference.from_dict(data.get("delegateReference", {})),
            target_class=data.get("targetClass", ""),
            spawn_class=data.get("spawnClass", ""),
            enum_type=data.get("enumType", ""),
            struct_type=data.get("structType", ""),
            macro_reference=data.get("macroReference", ""),
            timeline_name=data.get("timelineName", ""),
            literal_value=data.get("literalValue", ""),
            custom_event_name=data.get("customEventName", ""),
            input_action_name=data.get("inputActionName", ""),
            input_key=data.get("inputKey", ""),
            is_pure=data.get("bIsPure", False),
            is_latent=data.get("bIsLatent", False),
            self_context=data.get("bSelfContext", False),
            node_specific_data=data.get("nodeSpecificData", {})
        )

    @classmethod
    def create_call_function(cls, function_ref: MemberReference, x: float = 0, y: float = 0) -> "Node":
        """Create a K2Node_CallFunction node."""
        return cls(
            node_guid=str(uuid.uuid4()),
            node_class="K2Node_CallFunction",
            function_reference=function_ref,
            position_x=x,
            position_y=y
        )

    @classmethod
    def create_variable_get(cls, var_name: str, is_self: bool = True, x: float = 0, y: float = 0) -> "Node":
        """Create a K2Node_VariableGet node."""
        return cls(
            node_guid=str(uuid.uuid4()),
            node_class="K2Node_VariableGet",
            variable_reference=MemberReference(
                member_name=var_name,
                is_self_context=is_self
            ),
            position_x=x,
            position_y=y
        )

    @classmethod
    def create_variable_set(cls, var_name: str, is_self: bool = True, x: float = 0, y: float = 0) -> "Node":
        """Create a K2Node_VariableSet node."""
        return cls(
            node_guid=str(uuid.uuid4()),
            node_class="K2Node_VariableSet",
            variable_reference=MemberReference(
                member_name=var_name,
                is_self_context=is_self
            ),
            position_x=x,
            position_y=y
        )

    @classmethod
    def create_custom_event(cls, event_name: str, x: float = 0, y: float = 0) -> "Node":
        """Create a K2Node_CustomEvent node."""
        return cls(
            node_guid=str(uuid.uuid4()),
            node_class="K2Node_CustomEvent",
            custom_event_name=event_name,
            position_x=x,
            position_y=y
        )


# =============================================================================
# Graph Types
# =============================================================================

class GraphType(str, Enum):
    EVENT_GRAPH = "EventGraph"
    FUNCTION = "Function"
    MACRO = "Macro"


@dataclass
class Graph:
    """
    Represents a Blueprint graph.
    """
    graph_name: str = ""
    graph_type: GraphType = GraphType.EVENT_GRAPH
    graph_guid: str = ""
    nodes: List[Node] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "graphName": self.graph_name,
            "graphType": self.graph_type.value,
            "graphGuid": self.graph_guid,
            "nodes": [n.to_dict() for n in self.nodes]
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Graph":
        return cls(
            graph_name=data.get("graphName", ""),
            graph_type=GraphType(data.get("graphType", "EventGraph")),
            graph_guid=data.get("graphGuid", ""),
            nodes=[Node.from_dict(n) for n in data.get("nodes", [])]
        )


# =============================================================================
# Variable and Function Types
# =============================================================================

@dataclass
class Variable:
    """
    Represents a Blueprint variable.
    """
    var_name: str = ""
    var_guid: str = ""
    var_type: str = ""
    category: str = ""
    default_value: str = ""
    is_exposed: bool = False
    is_read_only: bool = False
    replication_condition: str = ""
    metadata: Dict[str, str] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "varName": self.var_name,
            "varGuid": self.var_guid,
            "varType": self.var_type,
            "category": self.category,
            "defaultValue": self.default_value,
            "bIsExposed": self.is_exposed,
            "bIsReadOnly": self.is_read_only,
            "replicationCondition": self.replication_condition,
            "metadata": self.metadata
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Variable":
        return cls(
            var_name=data.get("varName", ""),
            var_guid=data.get("varGuid", ""),
            var_type=data.get("varType", ""),
            category=data.get("category", ""),
            default_value=data.get("defaultValue", ""),
            is_exposed=data.get("bIsExposed", False),
            is_read_only=data.get("bIsReadOnly", False),
            replication_condition=data.get("replicationCondition", ""),
            metadata=data.get("metadata", {})
        )

    @classmethod
    def create_bool(cls, name: str, default: bool = False) -> "Variable":
        return cls(var_name=name, var_type="bool", default_value=str(default).lower())

    @classmethod
    def create_int(cls, name: str, default: int = 0) -> "Variable":
        return cls(var_name=name, var_type="int", default_value=str(default))

    @classmethod
    def create_float(cls, name: str, default: float = 0.0) -> "Variable":
        return cls(var_name=name, var_type="real", default_value=str(default))

    @classmethod
    def create_string(cls, name: str, default: str = "") -> "Variable":
        return cls(var_name=name, var_type="string", default_value=default)

    @classmethod
    def create_object(cls, name: str, object_class: str) -> "Variable":
        return cls(var_name=name, var_type=f"object:{object_class}")


@dataclass
class Function:
    """
    Represents a Blueprint function.
    """
    function_name: str = ""
    function_guid: str = ""
    graph: Graph = field(default_factory=Graph)
    parameters: List[Variable] = field(default_factory=list)
    return_values: List[Variable] = field(default_factory=list)
    is_pure: bool = False
    is_const: bool = False
    is_static: bool = False
    access_specifier: str = "Public"

    def to_dict(self) -> Dict[str, Any]:
        return {
            "functionName": self.function_name,
            "functionGuid": self.function_guid,
            "graph": self.graph.to_dict(),
            "parameters": [p.to_dict() for p in self.parameters],
            "returnValues": [r.to_dict() for r in self.return_values],
            "bIsPure": self.is_pure,
            "bIsConst": self.is_const,
            "bIsStatic": self.is_static,
            "accessSpecifier": self.access_specifier
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Function":
        return cls(
            function_name=data.get("functionName", ""),
            function_guid=data.get("functionGuid", ""),
            graph=Graph.from_dict(data.get("graph", {})),
            parameters=[Variable.from_dict(p) for p in data.get("parameters", [])],
            return_values=[Variable.from_dict(r) for r in data.get("returnValues", [])],
            is_pure=data.get("bIsPure", False),
            is_const=data.get("bIsConst", False),
            is_static=data.get("bIsStatic", False),
            access_specifier=data.get("accessSpecifier", "Public")
        )


# =============================================================================
# Component and Metadata Types
# =============================================================================

@dataclass
class Component:
    """
    Represents a Blueprint component.
    """
    component_name: str = ""
    component_class: str = ""
    parent_component: str = ""
    properties: Dict[str, str] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "componentName": self.component_name,
            "componentClass": self.component_class,
            "parentComponent": self.parent_component,
            "properties": self.properties
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Component":
        return cls(
            component_name=data.get("componentName", ""),
            component_class=data.get("componentClass", ""),
            parent_component=data.get("parentComponent", ""),
            properties=data.get("properties", {})
        )


@dataclass
class Metadata:
    """
    Blueprint metadata.
    """
    blueprint_name: str = ""
    blueprint_path: str = ""
    blueprint_type: str = "Normal"
    parent_class: str = ""
    generated_class: str = ""
    engine_version: str = ""
    serializer_version: str = "2.0.0"
    export_timestamp: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return {
            "blueprintName": self.blueprint_name,
            "blueprintPath": self.blueprint_path,
            "blueprintType": self.blueprint_type,
            "parentClass": self.parent_class,
            "generatedClass": self.generated_class,
            "engineVersion": self.engine_version,
            "serializerVersion": self.serializer_version,
            "exportTimestamp": self.export_timestamp
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Metadata":
        return cls(
            blueprint_name=data.get("blueprintName", ""),
            blueprint_path=data.get("blueprintPath", ""),
            blueprint_type=data.get("blueprintType", "Normal"),
            parent_class=data.get("parentClass", ""),
            generated_class=data.get("generatedClass", ""),
            engine_version=data.get("engineVersion", ""),
            serializer_version=data.get("serializerVersion", "2.0.0"),
            export_timestamp=data.get("exportTimestamp", "")
        )


# =============================================================================
# Blueprint Data
# =============================================================================

@dataclass
class BlueprintData:
    """
    Complete Blueprint JSON data structure.
    """
    metadata: Metadata = field(default_factory=Metadata)
    variables: List[Variable] = field(default_factory=list)
    event_graphs: List[Graph] = field(default_factory=list)
    functions: List[Function] = field(default_factory=list)
    macros: List[Graph] = field(default_factory=list)
    components: List[Component] = field(default_factory=list)
    implemented_interfaces: List[str] = field(default_factory=list)
    dependencies: List[str] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "metadata": self.metadata.to_dict(),
            "variables": [v.to_dict() for v in self.variables],
            "eventGraphs": [g.to_dict() for g in self.event_graphs],
            "functions": [f.to_dict() for f in self.functions],
            "macros": [m.to_dict() for m in self.macros],
            "components": [c.to_dict() for c in self.components],
            "implementedInterfaces": self.implemented_interfaces,
            "dependencies": self.dependencies
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "BlueprintData":
        return cls(
            metadata=Metadata.from_dict(data.get("metadata", {})),
            variables=[Variable.from_dict(v) for v in data.get("variables", [])],
            event_graphs=[Graph.from_dict(g) for g in data.get("eventGraphs", [])],
            functions=[Function.from_dict(f) for f in data.get("functions", [])],
            macros=[Graph.from_dict(m) for m in data.get("macros", [])],
            components=[Component.from_dict(c) for c in data.get("components", [])],
            implemented_interfaces=data.get("implementedInterfaces", []),
            dependencies=data.get("dependencies", [])
        )

    @classmethod
    def from_json(cls, json_str: str) -> "BlueprintData":
        return cls.from_dict(json.loads(json_str))

    @classmethod
    def from_file(cls, file_path: str) -> "BlueprintData":
        with open(file_path, 'r', encoding='utf-8') as f:
            return cls.from_json(f.read())

    def save_to_file(self, file_path: str, indent: int = 2):
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(self.to_json(indent))


# =============================================================================
# Common Node Type Constants
# =============================================================================

class NodeTypes:
    """Common Blueprint node class names."""
    CALL_FUNCTION = "K2Node_CallFunction"
    EVENT = "K2Node_Event"
    CUSTOM_EVENT = "K2Node_CustomEvent"
    VARIABLE_GET = "K2Node_VariableGet"
    VARIABLE_SET = "K2Node_VariableSet"
    DYNAMIC_CAST = "K2Node_DynamicCast"
    SPAWN_ACTOR = "K2Node_SpawnActorFromClass"
    TIMELINE = "K2Node_Timeline"
    MACRO_INSTANCE = "K2Node_MacroInstance"
    IF_THEN_ELSE = "K2Node_IfThenElse"
    SWITCH_ENUM = "K2Node_SwitchEnum"
    SWITCH_INTEGER = "K2Node_SwitchInteger"
    SWITCH_STRING = "K2Node_SwitchString"
    MAKE_ARRAY = "K2Node_MakeArray"
    MAKE_STRUCT = "K2Node_MakeStruct"
    BREAK_STRUCT = "K2Node_BreakStruct"
    FUNCTION_ENTRY = "K2Node_FunctionEntry"
    FUNCTION_RESULT = "K2Node_FunctionResult"
    CREATE_DELEGATE = "K2Node_CreateDelegate"
    CALL_DELEGATE = "K2Node_CallDelegate"
    ADD_DELEGATE = "K2Node_AddDelegate"
    EXECUTION_SEQUENCE = "K2Node_ExecutionSequence"
    MULTI_GATE = "K2Node_MultiGate"
    ASYNC_ACTION = "K2Node_AsyncAction"


class PinTypes:
    """Common Blueprint pin type strings."""
    EXEC = "exec"
    BOOL = "bool"
    BYTE = "byte"
    INT = "int"
    INT64 = "int64"
    FLOAT = "real"
    DOUBLE = "double"
    NAME = "name"
    STRING = "string"
    TEXT = "text"
    VECTOR = "struct:/Script/CoreUObject.Vector"
    ROTATOR = "struct:/Script/CoreUObject.Rotator"
    TRANSFORM = "struct:/Script/CoreUObject.Transform"
    OBJECT = "object"
    CLASS = "class"
    INTERFACE = "interface"
    SOFT_OBJECT = "softobject"
    SOFT_CLASS = "softclass"
    DELEGATE = "delegate"
    WILDCARD = "wildcard"


# =============================================================================
# Example Usage
# =============================================================================

if __name__ == "__main__":
    # Example: Create a simple Blueprint with a custom event that prints a message
    
    # Create metadata
    metadata = Metadata(
        blueprint_name="BP_ExampleActor",
        parent_class="/Script/Engine.Actor",
        blueprint_type="Normal"
    )

    # Create a variable
    my_variable = Variable.create_string("MyMessage", "Hello World")

    # Create nodes for the event graph
    custom_event = Node.create_custom_event("MyCustomEvent", x=0, y=0)
    
    print_string = Node.create_call_function(
        MemberReference.create_kismet_library_function("PrintString"),
        x=300, y=0
    )

    # Create the event graph
    event_graph = Graph(
        graph_name="EventGraph",
        graph_type=GraphType.EVENT_GRAPH,
        graph_guid=str(uuid.uuid4()),
        nodes=[custom_event, print_string]
    )

    # Assemble the Blueprint
    blueprint = BlueprintData(
        metadata=metadata,
        variables=[my_variable],
        event_graphs=[event_graph]
    )

    # Output as JSON
    print(blueprint.to_json())
