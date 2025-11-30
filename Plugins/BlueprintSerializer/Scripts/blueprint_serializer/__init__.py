#!/usr/bin/env python3
"""
Blueprint Serializer Python Automation Scripts

This module provides Python automation for the Blueprint Serializer plugin,
enabling Copilot MCP integration for Blueprint manipulation workflows.

Usage:
    python -m blueprint_serializer export <asset_path> [--output <file>]
    python -m blueprint_serializer import <json_file> <package_path> [--name <blueprint_name>]
    python -m blueprint_serializer diff <json_a> <json_b>
    python -m blueprint_serializer merge <original_bp> <modified_json> [--auto-open]
"""

import os

__version__ = "1.0.0"
__all__ = [
    "export_blueprint",
    "import_blueprint",
    "diff_blueprints",
    "merge_workflow",
    "UnrealRemote",
    "BlueprintGraph",
    "GraphNode",
    "GraphPin",
    "BlueprintGraphManager",
]
