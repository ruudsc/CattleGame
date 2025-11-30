#!/usr/bin/env python3
"""
Blueprint Graph Editor

In-memory graph representation for LLM-friendly Blueprint manipulation.
Provides structured commands for adding/removing/connecting nodes without
editing raw JSON directly.
"""

import json
import uuid
import warnings
from typing import Dict, List, Optional, Any, Tuple, Set
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class GraphPin:
    """Represents a pin on a Blueprint node."""
    pin_id: str
    pin_name: str
    direction: str  # "input" or "output"
    pin_type: str
    default_value: str = ""
    linked_to: List[str] = field(default_factory=list)  # List of connected pin IDs

    def to_dict(self) -> Dict[str, Any]:
        """Convert pin to dictionary format."""
        return {
            "pinId": self.pin_id,
            "pinName": self.pin_name,
            "direction": self.direction,
            "pinType": self.pin_type,
            "defaultValue": self.default_value,
            "linkedTo": self.linked_to.copy()
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'GraphPin':
        """Create pin from dictionary format."""
        return cls(
            pin_id=data["pinId"],
            pin_name=data["pinName"],
            direction=data["direction"],
            pin_type=data["pinType"],
            default_value=data.get("defaultValue", ""),
            linked_to=data.get("linkedTo", [])
        )


@dataclass
class GraphNode:
    """Represents a node in a Blueprint graph."""
    node_guid: str
    node_class: str
    node_title: str = ""
    node_comment: str = ""
    position_x: int = 0
    position_y: int = 0
    pins: Dict[str, GraphPin] = field(default_factory=dict)  # pin_name -> GraphPin
    properties: Dict[str, Any] = field(default_factory=dict)  # Additional node properties

    def to_dict(self) -> Dict[str, Any]:
        """Convert node to dictionary format."""
        result = {
            "nodeGuid": self.node_guid,
            "nodeClass": self.node_class,
            "nodeTitle": self.node_title,
            "nodeComment": self.node_comment,
            "positionX": self.position_x,
            "positionY": self.position_y,
            "pins": [pin.to_dict() for pin in self.pins.values()]
        }
        result.update(self.properties)
        return result

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'GraphNode':
        """Create node from dictionary format."""
        pins = {}
        for pin_data in data.get("pins", []):
            pin = GraphPin.from_dict(pin_data)
            pins[pin.pin_name] = pin

        properties = {k: v for k, v in data.items()
                     if k not in ["nodeGuid", "nodeClass", "nodeTitle", "nodeComment",
                                "positionX", "positionY", "pins"]}

        return cls(
            node_guid=data["nodeGuid"],
            node_class=data["nodeClass"],
            node_title=data.get("nodeTitle", ""),
            node_comment=data.get("nodeComment", ""),
            position_x=data.get("positionX", 0),
            position_y=data.get("positionY", 0),
            pins=pins,
            properties=properties
        )

    def get_pin(self, pin_name: str) -> Optional[GraphPin]:
        """Get a pin by name."""
        return self.pins.get(pin_name)

    def add_pin(self, pin: GraphPin) -> None:
        """Add a pin to the node."""
        self.pins[pin.pin_name] = pin

    def remove_pin(self, pin_name: str) -> bool:
        """Remove a pin from the node."""
        if pin_name in self.pins:
            del self.pins[pin_name]
            return True
        return False


@dataclass
class BlueprintGraph:
    """Represents a Blueprint graph with nodes, pins, and connections."""
    graph_name: str
    graph_type: str = "EventGraph"
    graph_guid: str = ""
    nodes: Dict[str, GraphNode] = field(default_factory=dict)  # node_guid -> GraphNode
    connections: Dict[str, Set[str]] = field(default_factory=dict)  # pin_id -> set of connected pin_ids

    def __post_init__(self):
        if not self.graph_guid:
            self.graph_guid = str(uuid.uuid4()).replace('-', '').upper()

    def to_dict(self) -> Dict[str, Any]:
        """Convert graph to dictionary format."""
        return {
            "graphName": self.graph_name,
            "graphType": self.graph_type,
            "graphGuid": self.graph_guid,
            "nodes": [node.to_dict() for node in self.nodes.values()]
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'BlueprintGraph':
        """Create graph from dictionary format."""
        nodes = {}
        for node_data in data.get("nodes", []):
            node = GraphNode.from_dict(node_data)
            nodes[node.node_guid] = node

        graph = cls(
            graph_name=data["graphName"],
            graph_type=data.get("graphType", "EventGraph"),
            graph_guid=data.get("graphGuid", "")
        )
        graph.nodes = nodes

        # Rebuild connections
        graph._rebuild_connections()

        return graph

    def _rebuild_connections(self) -> None:
        """Rebuild the connections dictionary from node pins."""
        self.connections = {}
        for node in self.nodes.values():
            for pin in node.pins.values():
                if pin.linked_to:
                    if pin.pin_id not in self.connections:
                        self.connections[pin.pin_id] = set()
                    self.connections[pin.pin_id].update(pin.linked_to)

    def add_node(self, node_class: str, position: Tuple[int, int] = (0, 0),
                 node_title: str = "", node_comment: str = "", **kwargs) -> str:
        """
        Add a new node to the graph.

        Args:
            node_class: The K2Node class name
            position: (x, y) position in the graph
            node_title: Display title for the node
            node_comment: Developer comment
            **kwargs: Additional node properties

        Returns:
            The node GUID of the newly created node
        """
        node_guid = str(uuid.uuid4()).replace('-', '').upper()

        node = GraphNode(
            node_guid=node_guid,
            node_class=node_class,
            node_title=node_title,
            node_comment=node_comment,
            position_x=position[0],
            position_y=position[1],
            properties=kwargs
        )

        self.nodes[node_guid] = node
        return node_guid

    def remove_node(self, node_guid: str) -> bool:
        """
        Remove a node from the graph.

        Args:
            node_guid: The GUID of the node to remove

        Returns:
            True if the node was removed, False if not found
        """
        if node_guid not in self.nodes:
            return False

        # Remove all connections involving this node's pins
        node = self.nodes[node_guid]
        for pin in node.pins.values():
            # Remove connections from this pin to others
            if pin.pin_id in self.connections:
                del self.connections[pin.pin_id]

            # Remove connections from other pins to this pin
            for other_pin_id, connected_pins in self.connections.items():
                connected_pins.discard(pin.pin_id)

        # Remove the node
        del self.nodes[node_guid]
        return True

    def connect_pins(self, from_node_guid: str, from_pin_name: str,
                    to_node_guid: str, to_pin_name: str) -> bool:
        """
        Connect two pins.

        Args:
            from_node_guid: Source node GUID
            from_pin_name: Source pin name
            to_node_guid: Target node GUID
            to_pin_name: Target pin name

        Returns:
            True if connection was made, False if invalid
        """
        from_node = self.nodes.get(from_node_guid)
        to_node = self.nodes.get(to_node_guid)

        if not from_node or not to_node:
            return False

        from_pin = from_node.get_pin(from_pin_name)
        to_pin = to_node.get_pin(to_pin_name)

        if not from_pin or not to_pin:
            return False

        # Check pin directions
        if from_pin.direction == to_pin.direction:
            return False  # Can't connect same direction pins

        # Add connection
        from_pin.linked_to.append(to_pin.pin_id)
        to_pin.linked_to.append(from_pin.pin_id)

        # Update connections dict
        if from_pin.pin_id not in self.connections:
            self.connections[from_pin.pin_id] = set()
        self.connections[from_pin.pin_id].add(to_pin.pin_id)

        if to_pin.pin_id not in self.connections:
            self.connections[to_pin.pin_id] = set()
        self.connections[to_pin.pin_id].add(from_pin.pin_id)

        return True

    def disconnect_pins(self, from_node_guid: str, from_pin_name: str,
                       to_node_guid: str, to_pin_name: str) -> bool:
        """
        Disconnect two pins.

        Args:
            from_node_guid: Source node GUID
            from_pin_name: Source pin name
            to_node_guid: Target node GUID
            to_pin_name: Target pin name

        Returns:
            True if connection was removed, False if not found
        """
        from_node = self.nodes.get(from_node_guid)
        to_node = self.nodes.get(to_node_guid)

        if not from_node or not to_node:
            return False

        from_pin = from_node.get_pin(from_pin_name)
        to_pin = to_node.get_pin(to_pin_name)

        if not from_pin or not to_pin:
            return False

        # Remove connection
        from_pin.linked_to = [pid for pid in from_pin.linked_to if pid != to_pin.pin_id]
        to_pin.linked_to = [pid for pid in to_pin.linked_to if pid != from_pin.pin_id]

        # Update connections dict
        if from_pin.pin_id in self.connections:
            self.connections[from_pin.pin_id].discard(to_pin.pin_id)
            if not self.connections[from_pin.pin_id]:
                del self.connections[from_pin.pin_id]

        if to_pin.pin_id in self.connections:
            self.connections[to_pin.pin_id].discard(from_pin.pin_id)
            if not self.connections[to_pin.pin_id]:
                del self.connections[to_pin.pin_id]

        return True

    def set_pin_default(self, node_guid: str, pin_name: str, value: str) -> bool:
        """
        Set the default value of a pin.

        Args:
            node_guid: Node GUID
            pin_name: Pin name
            value: New default value

        Returns:
            True if value was set, False if pin not found
        """
        node = self.nodes.get(node_guid)
        if not node:
            return False

        pin = node.get_pin(pin_name)
        if not pin:
            return False

        pin.default_value = value
        return True

    def get_node(self, node_guid: str) -> Optional[GraphNode]:
        """Get a node by GUID."""
        return self.nodes.get(node_guid)

    def find_nodes_by_class(self, node_class: str) -> List[GraphNode]:
        """Find all nodes of a specific class."""
        return [node for node in self.nodes.values() if node.node_class == node_class]

    def list_nodes(self) -> List[Dict[str, Any]]:
        """List all nodes with summary info."""
        return [{
            "guid": node.node_guid,
            "class": node.node_class,
            "title": node.node_title,
            "position": (node.position_x, node.position_y),
            "pins": len(node.pins)
        } for node in self.nodes.values()]


class BlueprintGraphManager:
    """Manages multiple graphs and provides high-level operations."""

    def __init__(self):
        self.graphs: Dict[str, BlueprintGraph] = {}
        self.schema: Dict[str, Any] = {}
        self._load_schema()

    def _load_schema(self) -> None:
        """Load the node schema for validation."""
        schema_path = Path(__file__).parent.parent / "schema.json"
        if schema_path.exists():
            try:
                with open(schema_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    self.schema = {node["nodeClass"]: node for node in data.get("nodeSchemas", [])}
            except Exception as e:
                warnings.warn(f"Failed to load schema: {e}")

    def load_graph(self, json_path: str, graph_name: str = "EventGraph") -> bool:
        """
        Load a graph from JSON file.

        Args:
            json_path: Path to the JSON file
            graph_name: Name of the graph to load (for multi-graph Blueprints)

        Returns:
            True if loaded successfully
        """
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                data = json.load(f)

            # Find the specified graph
            graph_data = None
            if "eventGraphs" in data:
                for graph in data["eventGraphs"]:
                    if graph["graphName"] == graph_name:
                        graph_data = graph
                        break

            if not graph_data and "functions" in data:
                for func in data["functions"]:
                    if func["functionName"] == graph_name:
                        graph_data = func["graph"]
                        break

            if not graph_data:
                warnings.warn(f"Graph '{graph_name}' not found in {json_path}")
                return False

            graph = BlueprintGraph.from_dict(graph_data)
            self.graphs[graph_name] = graph
            return True

        except Exception as e:
            warnings.warn(f"Failed to load graph from {json_path}: {e}")
            return False

    def save_graph(self, graph_name: str, json_path: str) -> bool:
        """
        Save a graph to JSON file.

        Args:
            graph_name: Name of the graph to save
            json_path: Path to save the JSON file

        Returns:
            True if saved successfully
        """
        if graph_name not in self.graphs:
            return False

        try:
            # For now, just save the graph data
            # In a full implementation, this would merge back into the full Blueprint JSON
            graph_data = self.graphs[graph_name].to_dict()

            with open(json_path, 'w', encoding='utf-8') as f:
                json.dump(graph_data, f, indent=2)

            return True

        except Exception as e:
            warnings.warn(f"Failed to save graph to {json_path}: {e}")
            return False

    def create_node_factory(self, node_class: str) -> Optional[Dict[str, Any]]:
        """
        Create a node factory based on schema.

        Args:
            node_class: The node class to create a factory for

        Returns:
            Factory info or None if not found in schema
        """
        if node_class not in self.schema:
            warnings.warn(f"Node class '{node_class}' not found in schema")
            return None

        schema_info = self.schema[node_class]
        return {
            "class": node_class,
            "display_name": schema_info.get("displayName", node_class),
            "category": schema_info.get("category", "Unknown"),
            "description": schema_info.get("description", ""),
            "properties": schema_info.get("properties", []),
            "has_dynamic_pins": schema_info.get("hasDynamicPins", False),
            "is_latent": schema_info.get("isLatent", False),
            "can_be_pure": schema_info.get("canBePure", False)
        }

    def validate_node_class(self, node_class: str) -> bool:
        """
        Validate if a node class exists in schema.

        Args:
            node_class: Node class to validate

        Returns:
            True if valid (or schema not loaded), False if invalid
        """
        if not self.schema:
            return True  # Allow if no schema loaded
        return node_class in self.schema

    def get_graph(self, graph_name: str) -> Optional[BlueprintGraph]:
        """Get a graph by name."""
        return self.graphs.get(graph_name)

    def list_graphs(self) -> List[str]:
        """List all loaded graphs."""
        return list(self.graphs.keys())