#!/usr/bin/env python3
"""
Diff Blueprints

CLI tool to compare two Blueprint JSON files and generate a human-readable diff summary.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional


def load_blueprint_json(path: str) -> Dict[str, Any]:
    """Load and parse a Blueprint JSON file."""
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def diff_lists(
    list_a: List[Dict],
    list_b: List[Dict],
    key_field: str,
    item_name: str
) -> List[str]:
    """
    Compare two lists of items by a key field.
    
    Returns a list of diff descriptions.
    """
    diffs = []
    
    # Build lookup dictionaries
    items_a = {item.get(key_field): item for item in list_a}
    items_b = {item.get(key_field): item for item in list_b}
    
    keys_a = set(items_a.keys())
    keys_b = set(items_b.keys())
    
    # Added items
    for key in keys_b - keys_a:
        diffs.append(f"+ {item_name}: {key}")
    
    # Removed items
    for key in keys_a - keys_b:
        diffs.append(f"- {item_name}: {key}")
    
    # Modified items
    for key in keys_a & keys_b:
        item_a = items_a[key]
        item_b = items_b[key]
        
        # Compare item properties (simplified)
        if item_a != item_b:
            diffs.append(f"~ {item_name}: {key} (modified)")
    
    return diffs


def diff_graphs(
    graphs_a: List[Dict],
    graphs_b: List[Dict]
) -> List[str]:
    """Compare event graphs between two Blueprints."""
    diffs = []
    
    # Build lookup by graph name
    graphs_by_name_a = {g.get("GraphName"): g for g in graphs_a}
    graphs_by_name_b = {g.get("GraphName"): g for g in graphs_b}
    
    names_a = set(graphs_by_name_a.keys())
    names_b = set(graphs_by_name_b.keys())
    
    # Added/removed graphs
    for name in names_b - names_a:
        graph = graphs_by_name_b[name]
        node_count = len(graph.get("Nodes", []))
        diffs.append(f"+ Graph: {name} ({node_count} nodes)")
    
    for name in names_a - names_b:
        graph = graphs_by_name_a[name]
        node_count = len(graph.get("Nodes", []))
        diffs.append(f"- Graph: {name} ({node_count} nodes)")
    
    # Compare common graphs
    for name in names_a & names_b:
        graph_a = graphs_by_name_a[name]
        graph_b = graphs_by_name_b[name]
        
        nodes_a = graph_a.get("Nodes", [])
        nodes_b = graph_b.get("Nodes", [])
        
        # Compare by node GUID
        guids_a = {n.get("NodeGuid"): n for n in nodes_a}
        guids_b = {n.get("NodeGuid"): n for n in nodes_b}
        
        added = set(guids_b.keys()) - set(guids_a.keys())
        removed = set(guids_a.keys()) - set(guids_b.keys())
        
        if added or removed:
            diffs.append(f"~ Graph: {name}")
            for guid in added:
                node = guids_b[guid]
                diffs.append(f"    + Node: {node.get('NodeTitle', node.get('NodeClass'))}")
            for guid in removed:
                node = guids_a[guid]
                diffs.append(f"    - Node: {node.get('NodeTitle', node.get('NodeClass'))}")
    
    return diffs


def diff_blueprints(json_path_a: str, json_path_b: str) -> Dict[str, Any]:
    """
    Compare two Blueprint JSON files.
    
    Args:
        json_path_a: Path to first Blueprint JSON
        json_path_b: Path to second Blueprint JSON
        
    Returns:
        Dictionary with diff results including 'summary' and 'details'
    """
    try:
        bp_a = load_blueprint_json(json_path_a)
        bp_b = load_blueprint_json(json_path_b)
    except Exception as e:
        return {"success": False, "error": str(e)}
    
    diffs = []
    
    # Metadata comparison
    meta_a = bp_a.get("Metadata", {})
    meta_b = bp_b.get("Metadata", {})
    
    if meta_a.get("ParentClass") != meta_b.get("ParentClass"):
        diffs.append(f"~ ParentClass: {meta_a.get('ParentClass')} -> {meta_b.get('ParentClass')}")
    
    # Variables
    vars_a = bp_a.get("Variables", [])
    vars_b = bp_b.get("Variables", [])
    diffs.extend(diff_lists(vars_a, vars_b, "VarName", "Variable"))
    
    # Functions
    funcs_a = bp_a.get("Functions", [])
    funcs_b = bp_b.get("Functions", [])
    diffs.extend(diff_lists(funcs_a, funcs_b, "FunctionName", "Function"))
    
    # Components
    comps_a = bp_a.get("Components", [])
    comps_b = bp_b.get("Components", [])
    diffs.extend(diff_lists(comps_a, comps_b, "ComponentName", "Component"))
    
    # Event Graphs
    graphs_a = bp_a.get("EventGraphs", [])
    graphs_b = bp_b.get("EventGraphs", [])
    diffs.extend(diff_graphs(graphs_a, graphs_b))
    
    # Interfaces
    interfaces_a = set(bp_a.get("ImplementedInterfaces", []))
    interfaces_b = set(bp_b.get("ImplementedInterfaces", []))
    
    for iface in interfaces_b - interfaces_a:
        diffs.append(f"+ Interface: {iface}")
    for iface in interfaces_a - interfaces_b:
        diffs.append(f"- Interface: {iface}")
    
    # Build summary
    additions = sum(1 for d in diffs if d.startswith('+'))
    removals = sum(1 for d in diffs if d.startswith('-'))
    modifications = sum(1 for d in diffs if d.startswith('~'))
    
    summary = f"Comparing {meta_a.get('BlueprintName', 'Unknown')} vs {meta_b.get('BlueprintName', 'Unknown')}\n"
    summary += f"Changes: +{additions} added, -{removals} removed, ~{modifications} modified\n"
    summary += "-" * 60 + "\n"
    summary += "\n".join(diffs) if diffs else "No differences found."
    
    return {
        "success": True,
        "summary": summary,
        "details": {
            "additions": additions,
            "removals": removals,
            "modifications": modifications,
            "diffs": diffs
        }
    }


def main():
    """CLI entry point for diff_blueprints."""
    parser = argparse.ArgumentParser(
        description="Compare two Blueprint JSON files and show differences"
    )
    parser.add_argument(
        "json_a",
        help="Path to first Blueprint JSON file"
    )
    parser.add_argument(
        "json_b",
        help="Path to second Blueprint JSON file"
    )
    parser.add_argument(
        "-q", "--quiet",
        action="store_true",
        help="Only output summary line"
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output as JSON"
    )
    
    args = parser.parse_args()
    
    result = diff_blueprints(args.json_a, args.json_b)
    
    if not result.get("success"):
        print(f"Error: {result.get('error')}", file=sys.stderr)
        return 1
    
    if args.json:
        print(json.dumps(result, indent=2))
    elif args.quiet:
        details = result.get("details", {})
        print(f"+{details.get('additions', 0)} -{details.get('removals', 0)} ~{details.get('modifications', 0)}")
    else:
        print(result.get("summary", ""))
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
