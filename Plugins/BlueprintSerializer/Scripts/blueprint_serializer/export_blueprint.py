#!/usr/bin/env python3
"""
Export Blueprint to JSON

CLI tool to export a Blueprint asset to AI-readable JSON format.
"""

import argparse
import sys
import os
from pathlib import Path
from typing import Optional

from .unreal_remote import UnrealRemote, get_default_remote


def export_blueprint(
    asset_path: str,
    output_path: Optional[str] = None,
    remote: Optional[UnrealRemote] = None,
    pretty: bool = True
) -> dict:
    """
    Export a Blueprint asset to JSON.
    
    Args:
        asset_path: Unreal asset path (e.g., '/Game/Blueprints/BP_MyActor')
        output_path: Output JSON file path (default: <blueprint_name>.json in current dir)
        remote: UnrealRemote instance (default: auto-configured)
        pretty: Whether to format JSON with indentation
        
    Returns:
        Dictionary with 'success' and 'output_path' or 'error' keys
    """
    if remote is None:
        remote = get_default_remote()
    
    # Generate default output path if not specified
    if output_path is None:
        blueprint_name = asset_path.split('/')[-1]
        output_path = f"{blueprint_name}.json"
    
    # Ensure output directory exists
    output_dir = Path(output_path).parent
    if output_dir and not output_dir.exists():
        output_dir.mkdir(parents=True, exist_ok=True)
    
    # Connect and export
    if not remote.connect():
        return {
            "success": False,
            "error": "Could not connect to Unreal Editor. Make sure it's running with Remote Execution enabled."
        }
    
    try:
        result = remote.export_blueprint(asset_path, str(Path(output_path).absolute()))
        return result
    finally:
        remote.disconnect()


def main():
    """CLI entry point for export_blueprint."""
    parser = argparse.ArgumentParser(
        description="Export a Blueprint asset to AI-readable JSON format"
    )
    parser.add_argument(
        "asset_path",
        help="Unreal asset path (e.g., '/Game/Blueprints/BP_MyActor')"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output JSON file path (default: <blueprint_name>.json)",
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
    result = export_blueprint(args.asset_path, args.output, remote)
    
    if result.get("success"):
        print(f"Successfully exported to: {result.get('output_path')}")
        return 0
    else:
        print(f"Error: {result.get('error')}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
