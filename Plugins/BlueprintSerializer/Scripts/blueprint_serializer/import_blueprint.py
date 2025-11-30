#!/usr/bin/env python3
"""
Import Blueprint from JSON

CLI tool to create a Blueprint asset from JSON format.
"""

import argparse
import sys
import os
from pathlib import Path
from typing import Optional

from .unreal_remote import UnrealRemote, get_default_remote


def import_blueprint(
    json_path: str,
    package_path: str,
    blueprint_name: Optional[str] = None,
    remote: Optional[UnrealRemote] = None
) -> dict:
    """
    Import a Blueprint from JSON file.
    
    Args:
        json_path: Path to the JSON file
        package_path: Unreal package path (e.g., '/Game/Blueprints')
        blueprint_name: Name for the Blueprint (default: from JSON metadata)
        remote: UnrealRemote instance (default: auto-configured)
        
    Returns:
        Dictionary with 'success' and 'asset_path' or 'error' keys
    """
    if remote is None:
        remote = get_default_remote()
    
    # Verify JSON file exists
    json_file = Path(json_path)
    if not json_file.exists():
        return {
            "success": False,
            "error": f"JSON file not found: {json_path}"
        }
    
    # Connect and import
    if not remote.connect():
        return {
            "success": False,
            "error": "Could not connect to Unreal Editor. Make sure it's running with Remote Execution enabled."
        }
    
    try:
        result = remote.import_blueprint(
            str(json_file.absolute()),
            package_path,
            blueprint_name
        )
        return result
    finally:
        remote.disconnect()


def main():
    """CLI entry point for import_blueprint."""
    parser = argparse.ArgumentParser(
        description="Create a Blueprint asset from JSON format"
    )
    parser.add_argument(
        "json_path",
        help="Path to the JSON file"
    )
    parser.add_argument(
        "package_path",
        help="Unreal package path (e.g., '/Game/Blueprints')"
    )
    parser.add_argument(
        "-n", "--name",
        help="Name for the Blueprint (default: from JSON metadata)",
        default=None
    )
    parser.add_argument(
        "--host",
        help="Unreal Remote Execution host",
        default=os.environ.get("UNREAL_REMOTE_HOST", "127.0.0.1")
    )
    parser.add_argument(
        "--port",
        help="Unreal Remote Execution port",
        type=int,
        default=int(os.environ.get("UNREAL_REMOTE_PORT", "6776"))
    )
    
    args = parser.parse_args()
    
    remote = UnrealRemote(host=args.host, port=args.port)
    result = import_blueprint(args.json_path, args.package_path, args.name, remote)
    
    if result.get("success"):
        print(f"Successfully created Blueprint: {result.get('asset_path')}")
        return 0
    else:
        print(f"Error: {result.get('error')}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
