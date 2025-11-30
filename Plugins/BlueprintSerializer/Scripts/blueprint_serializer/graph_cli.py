#!/usr/bin/env python3
"""
Blueprint Graph CLI

Command-line interface for manipulating Blueprint graphs.
Provides structured commands for LLMs to modify Blueprints without editing raw JSON.
"""

import argparse
import sys
import json
from pathlib import Path
from typing import Optional

from .graph_editor import BlueprintGraphManager, BlueprintGraph


class BlueprintGraphCLI:
    """Command-line interface for Blueprint graph manipulation."""

    def __init__(self):
        self.manager = BlueprintGraphManager()
        self.current_graph: Optional[str] = None

    def cmd_load(self, args) -> int:
        """Load a Blueprint graph from JSON file."""
        json_path = args.json_path
        graph_name = args.graph_name or "EventGraph"

        if not Path(json_path).exists():
            print(f"Error: File {json_path} does not exist")
            return 1

        if self.manager.load_graph(json_path, graph_name):
            self.current_graph = graph_name
            graph = self.manager.get_graph(graph_name)
            if graph:
                print(f"Loaded graph '{graph_name}' with {len(graph.nodes)} nodes")
                return 0
        else:
            print(f"Error: Failed to load graph '{graph_name}' from {json_path}")
            return 1

        return 1  # Fallback

    def cmd_save(self, args) -> int:
        """Save the current graph to JSON file."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        output_path = args.output_path
        if self.manager.save_graph(self.current_graph, output_path):
            print(f"Saved graph '{self.current_graph}' to {output_path}")
            return 0
        else:
            print(f"Error: Failed to save graph to {output_path}")
            return 1

    def cmd_add_node(self, args) -> int:
        """Add a new node to the current graph."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        # Validate node class
        if not self.manager.validate_node_class(args.node_class):
            print(f"Warning: Node class '{args.node_class}' not found in schema")

        # Parse position
        position = (args.pos_x or 0, args.pos_y or 0)

        # Add node
        node_guid = graph.add_node(
            node_class=args.node_class,
            position=position,
            node_title=getattr(args, 'title', ''),
            node_comment=getattr(args, 'comment', '')
        )

        print(f"Added node {node_guid} of class '{args.node_class}' at position {position}")
        return 0

    def cmd_remove_node(self, args) -> int:
        """Remove a node from the current graph."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        if graph.remove_node(args.node_guid):
            print(f"Removed node {args.node_guid}")
            return 0
        else:
            print(f"Error: Node {args.node_guid} not found")
            return 1

    def cmd_connect(self, args) -> int:
        """Connect two pins."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        # Parse pin specifications: node_guid.pin_name
        try:
            from_parts = args.from_pin.split('.')
            to_parts = args.to_pin.split('.')

            if len(from_parts) != 2 or len(to_parts) != 2:
                print("Error: Pin specification must be in format 'node_guid.pin_name'")
                return 1

            from_node_guid, from_pin_name = from_parts
            to_node_guid, to_pin_name = to_parts

        except ValueError:
            print("Error: Invalid pin specification format")
            return 1

        if graph.connect_pins(from_node_guid, from_pin_name, to_node_guid, to_pin_name):
            print(f"Connected {args.from_pin} -> {args.to_pin}")
            return 0
        else:
            print(f"Error: Failed to connect pins {args.from_pin} -> {args.to_pin}")
            return 1

    def cmd_disconnect(self, args) -> int:
        """Disconnect two pins."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        # Parse pin specifications: node_guid.pin_name
        try:
            from_parts = args.from_pin.split('.')
            to_parts = args.to_pin.split('.')

            if len(from_parts) != 2 or len(to_parts) != 2:
                print("Error: Pin specification must be in format 'node_guid.pin_name'")
                return 1

            from_node_guid, from_pin_name = from_parts
            to_node_guid, to_pin_name = to_parts

        except ValueError:
            print("Error: Invalid pin specification format")
            return 1

        if graph.disconnect_pins(from_node_guid, from_pin_name, to_node_guid, to_pin_name):
            print(f"Disconnected {args.from_pin} -> {args.to_pin}")
            return 0
        else:
            print(f"Error: Failed to disconnect pins {args.from_pin} -> {args.to_pin}")
            return 1

    def cmd_set_default(self, args) -> int:
        """Set the default value of a pin."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        # Parse pin specification: node_guid.pin_name
        try:
            parts = args.pin.split('.')
            if len(parts) != 2:
                print("Error: Pin specification must be in format 'node_guid.pin_name'")
                return 1

            node_guid, pin_name = parts

        except ValueError:
            print("Error: Invalid pin specification format")
            return 1

        if graph.set_pin_default(node_guid, pin_name, args.value):
            print(f"Set default value of {args.pin} to '{args.value}'")
            return 0
        else:
            print(f"Error: Failed to set default value for pin {args.pin}")
            return 1

    def cmd_list_nodes(self, args) -> int:
        """List all nodes in the current graph."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        nodes = graph.list_nodes()
        if not nodes:
            print("No nodes in graph")
            return 0

        print(f"Nodes in graph '{self.current_graph}':")
        print("-" * 80)
        for node in nodes:
            print(f"GUID: {node['guid']}")
            print(f"Class: {node['class']}")
            print(f"Title: {node['title']}")
            print(f"Position: {node['position']}")
            print(f"Pins: {node['pins']}")
            print("-" * 40)
        return 0

    def cmd_show_graph(self, args) -> int:
        """Show summary of the current graph."""
        if not self.current_graph:
            print("Error: No graph loaded. Use 'graph load <json>' first.")
            return 1

        graph = self.manager.get_graph(self.current_graph)
        if not graph:
            print("Error: Current graph not found")
            return 1

        print(f"Graph: {self.current_graph}")
        print(f"Type: {graph.graph_type}")
        print(f"GUID: {graph.graph_guid}")
        print(f"Nodes: {len(graph.nodes)}")
        print(f"Connections: {len(graph.connections)}")

        # Show node classes
        node_classes = {}
        for node in graph.nodes.values():
            node_classes[node.node_class] = node_classes.get(node.node_class, 0) + 1

        if node_classes:
            print("Node classes:")
            for cls, count in sorted(node_classes.items()):
                print(f"  {cls}: {count}")

        return 0


def create_parser() -> argparse.ArgumentParser:
    """Create the argument parser."""
    parser = argparse.ArgumentParser(
        prog="bp-graph",
        description="Blueprint Graph Editor - LLM-friendly Blueprint manipulation"
    )

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Load command
    load_parser = subparsers.add_parser(
        "load",
        help="Load a Blueprint graph from JSON"
    )
    load_parser.add_argument("json_path", help="Path to the Blueprint JSON file")
    load_parser.add_argument(
        "--graph",
        dest="graph_name",
        default="EventGraph",
        help="Name of the graph to load (default: EventGraph)"
    )

    # Save command
    save_parser = subparsers.add_parser(
        "save",
        help="Save the current graph to JSON"
    )
    save_parser.add_argument("output_path", help="Path to save the graph JSON")

    # Add node command
    add_parser = subparsers.add_parser(
        "add-node",
        help="Add a new node to the current graph"
    )
    add_parser.add_argument("node_class", help="The K2Node class name")
    add_parser.add_argument(
        "--pos",
        nargs=2,
        type=int,
        metavar=("X", "Y"),
        help="Position as X Y coordinates"
    )
    add_parser.add_argument("--title", help="Node title")
    add_parser.add_argument("--comment", help="Node comment")

    # Remove node command
    remove_parser = subparsers.add_parser(
        "remove-node",
        help="Remove a node from the current graph"
    )
    remove_parser.add_argument("node_guid", help="GUID of the node to remove")

    # Connect command
    connect_parser = subparsers.add_parser(
        "connect",
        help="Connect two pins"
    )
    connect_parser.add_argument(
        "from_pin",
        help="Source pin in format 'node_guid.pin_name'"
    )
    connect_parser.add_argument(
        "to_pin",
        help="Target pin in format 'node_guid.pin_name'"
    )

    # Disconnect command
    disconnect_parser = subparsers.add_parser(
        "disconnect",
        help="Disconnect two pins"
    )
    disconnect_parser.add_argument(
        "from_pin",
        help="Source pin in format 'node_guid.pin_name'"
    )
    disconnect_parser.add_argument(
        "to_pin",
        help="Target pin in format 'node_guid.pin_name'"
    )

    # Set default command
    set_default_parser = subparsers.add_parser(
        "set-default",
        help="Set the default value of a pin"
    )
    set_default_parser.add_argument(
        "pin",
        help="Pin in format 'node_guid.pin_name'"
    )
    set_default_parser.add_argument("value", help="New default value")

    # List nodes command
    subparsers.add_parser(
        "list-nodes",
        help="List all nodes in the current graph"
    )

    # Show graph command
    subparsers.add_parser(
        "show-graph",
        help="Show summary of the current graph"
    )

    return parser


def main():
    """Main CLI entry point."""
    parser = create_parser()
    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 0

    cli = BlueprintGraphCLI()

    # Route to appropriate command
    command_map = {
        "load": cli.cmd_load,
        "save": cli.cmd_save,
        "add-node": cli.cmd_add_node,
        "remove-node": cli.cmd_remove_node,
        "connect": cli.cmd_connect,
        "disconnect": cli.cmd_disconnect,
        "set-default": cli.cmd_set_default,
        "list-nodes": cli.cmd_list_nodes,
        "show-graph": cli.cmd_show_graph,
    }

    if args.command in command_map:
        return command_map[args.command](args)
    else:
        print(f"Unknown command: {args.command}")
        return 1


if __name__ == "__main__":
    sys.exit(main())