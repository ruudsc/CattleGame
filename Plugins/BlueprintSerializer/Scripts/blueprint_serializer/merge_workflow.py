#!/usr/bin/env python3
"""
Blueprint Merge Workflow

Orchestrates the complete AI-assisted Blueprint modification workflow:
1. Export original Blueprint to JSON
2. Apply AI modifications (placeholder for Copilot)
3. Import modified JSON as new Blueprint
4. Open merge editor to combine changes
"""

import argparse
import json
import shutil
import sys
import tempfile
from pathlib import Path
from typing import Optional, Dict, Any

from .unreal_remote import UnrealRemote, get_default_remote
from .export_blueprint import export_blueprint
from .import_blueprint import import_blueprint
from .diff_blueprints import diff_blueprints


class MergeWorkflow:
    """
    Manages the AI-assisted Blueprint modification workflow.
    """
    
    def __init__(self, remote: Optional[UnrealRemote] = None):
        """
        Initialize the merge workflow.
        
        Args:
            remote: UnrealRemote instance for editor communication
        """
        self.remote = remote or get_default_remote()
        self.temp_dir = Path(tempfile.mkdtemp(prefix="blueprint_merge_"))
    
    def cleanup(self):
        """Clean up temporary files."""
        if self.temp_dir.exists():
            shutil.rmtree(self.temp_dir, ignore_errors=True)
    
    def run_full_workflow(
        self,
        original_asset_path: str,
        modified_json_path: str,
        auto_open_merge: bool = True
    ) -> Dict[str, Any]:
        """
        Run the complete merge workflow.
        
        Args:
            original_asset_path: Unreal path to original Blueprint
            modified_json_path: Path to AI-modified JSON file
            auto_open_merge: Whether to automatically open merge editor
            
        Returns:
            Workflow result dictionary
        """
        results = {
            "success": False,
            "steps": {}
        }
        
        try:
            # Step 1: Export original Blueprint
            print(f"Step 1: Exporting original Blueprint: {original_asset_path}")
            original_json_path = self.temp_dir / "original.json"
            
            export_result = export_blueprint(
                original_asset_path,
                str(original_json_path),
                self.remote
            )
            
            results["steps"]["export"] = export_result
            
            if not export_result.get("success"):
                print(f"  Failed: {export_result.get('error')}")
                return results
            
            print(f"  Exported to: {original_json_path}")
            
            # Step 2: Diff the blueprints
            print(f"Step 2: Comparing original with modified...")
            diff_result = diff_blueprints(str(original_json_path), modified_json_path)
            results["steps"]["diff"] = diff_result
            
            if diff_result.get("success"):
                details = diff_result.get("details", {})
                print(f"  Changes: +{details.get('additions', 0)} -{details.get('removals', 0)} ~{details.get('modifications', 0)}")
            
            # Step 3: Import modified JSON as new Blueprint
            print(f"Step 3: Importing modified Blueprint...")
            
            # Load modified JSON to get the name
            with open(modified_json_path, 'r', encoding='utf-8') as f:
                modified_data = json.load(f)
            
            original_name = modified_data.get("Metadata", {}).get("BlueprintName", "Blueprint")
            modified_name = f"{original_name}_AIModified"
            
            # Get the package path from the original asset path
            package_path = "/".join(original_asset_path.split("/")[:-1])
            
            import_result = import_blueprint(
                modified_json_path,
                package_path,
                modified_name,
                self.remote
            )
            
            results["steps"]["import"] = import_result
            
            if not import_result.get("success"):
                print(f"  Failed: {import_result.get('error')}")
                return results
            
            modified_asset_path = import_result.get("asset_path")
            print(f"  Created: {modified_asset_path}")
            
            # Step 4: Open merge editor
            if auto_open_merge:
                print(f"Step 4: Opening merge editor...")
                
                if not self.remote.connect():
                    results["steps"]["merge"] = {
                        "success": False,
                        "error": "Could not connect to Unreal Editor"
                    }
                    return results
                
                try:
                    merge_result = self.remote.open_merge_editor(
                        original_asset_path,
                        modified_asset_path
                    )
                    results["steps"]["merge"] = merge_result
                    
                    if merge_result.get("success"):
                        print("  Merge editor opened successfully")
                    else:
                        print(f"  Failed: {merge_result.get('error')}")
                finally:
                    self.remote.disconnect()
            else:
                print(f"Step 4: Skipped (auto_open_merge=False)")
                print(f"  Original: {original_asset_path}")
                print(f"  Modified: {modified_asset_path}")
                results["steps"]["merge"] = {"skipped": True}
            
            results["success"] = True
            results["original_asset"] = original_asset_path
            results["modified_asset"] = modified_asset_path
            
            return results
            
        except Exception as e:
            results["error"] = str(e)
            return results
        finally:
            # Clean up temp files
            self.cleanup()
    
    def prepare_for_ai(
        self,
        asset_path: str,
        output_json_path: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Export a Blueprint for AI modification.
        
        Args:
            asset_path: Unreal path to Blueprint
            output_json_path: Where to save the JSON (default: current dir)
            
        Returns:
            Export result with instructions for AI
        """
        if output_json_path is None:
            name = asset_path.split("/")[-1]
            output_json_path = f"{name}.json"
        
        result = export_blueprint(asset_path, output_json_path, self.remote)
        
        if result.get("success"):
            result["instructions"] = f"""
Blueprint exported to: {output_json_path}

To complete the AI modification workflow:
1. Modify the JSON file with your changes
2. Run: python -m blueprint_serializer merge {asset_path} {output_json_path}

The merge editor will open allowing you to review and accept changes.
"""
        
        return result

    def merge_directly(
        self,
        target_asset_path: str,
        json_path: str
    ) -> Dict[str, Any]:
        """
        Merge JSON directly into an existing Blueprint without creating a new asset.
        This adds new nodes/variables without removing existing ones.
        
        Args:
            target_asset_path: Unreal path to target Blueprint
            json_path: Path to JSON file with new content
            
        Returns:
            Merge result dictionary
        """
        results = {
            "success": False,
        }
        
        try:
            print(f"Merging JSON into: {target_asset_path}")
            print(f"JSON source: {json_path}")
            
            if not self.remote.connect():
                results["error"] = "Could not connect to Unreal Editor. Make sure it's running with Remote Execution enabled."
                return results
            
            try:
                merge_result = self.remote.merge_json_into_blueprint(
                    json_path,
                    target_asset_path
                )
                
                if merge_result.get("success"):
                    print(f"  Successfully merged into: {merge_result.get('asset_path')}")
                    results["success"] = True
                    results["asset_path"] = merge_result.get("asset_path")
                else:
                    results["error"] = merge_result.get("error", "Unknown error during merge")
                    print(f"  Failed: {results['error']}")
                    
            finally:
                self.remote.disconnect()
                
            return results
            
        except Exception as e:
            results["error"] = str(e)
            return results


def main():
    """CLI entry point for merge_workflow."""
    parser = argparse.ArgumentParser(
        description="Orchestrate AI-assisted Blueprint modification workflow"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Commands")
    
    # Prepare command
    prepare_parser = subparsers.add_parser(
        "prepare",
        help="Export Blueprint for AI modification"
    )
    prepare_parser.add_argument(
        "asset_path",
        help="Unreal asset path to Blueprint"
    )
    prepare_parser.add_argument(
        "-o", "--output",
        help="Output JSON file path",
        default=None
    )
    
    # Merge command (creates new BP + opens merge editor)
    merge_parser = subparsers.add_parser(
        "merge",
        help="Import modified JSON and open merge editor"
    )
    merge_parser.add_argument(
        "original_asset",
        help="Unreal path to original Blueprint"
    )
    merge_parser.add_argument(
        "modified_json",
        help="Path to modified JSON file"
    )
    merge_parser.add_argument(
        "--no-auto-merge",
        action="store_true",
        help="Don't automatically open merge editor"
    )
    
    # Direct merge command (merges into existing BP)
    direct_parser = subparsers.add_parser(
        "direct",
        help="Merge JSON directly into existing Blueprint (adds new nodes)"
    )
    direct_parser.add_argument(
        "target_asset",
        help="Unreal path to target Blueprint"
    )
    direct_parser.add_argument(
        "json_file",
        help="Path to JSON file with new content"
    )
    
    # Common arguments
    for subparser in [prepare_parser, merge_parser, direct_parser]:
        subparser.add_argument(
            "--host",
            help="Unreal Remote Execution host",
            default="127.0.0.1"
        )
        subparser.add_argument(
            "--port",
            help="Unreal Remote Execution port",
            type=int,
            default=6776
        )
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    remote = UnrealRemote(host=args.host, port=args.port)
    workflow = MergeWorkflow(remote)
    
    try:
        if args.command == "prepare":
            result = workflow.prepare_for_ai(args.asset_path, args.output)
            if result.get("success"):
                print(result.get("instructions", ""))
                return 0
            else:
                print(f"Error: {result.get('error')}", file=sys.stderr)
                return 1
        
        elif args.command == "merge":
            result = workflow.run_full_workflow(
                args.original_asset,
                args.modified_json,
                auto_open_merge=not args.no_auto_merge
            )
            if result.get("success"):
                print("\nWorkflow completed successfully!")
                return 0
            else:
                print(f"\nWorkflow failed: {result.get('error', 'Unknown error')}", file=sys.stderr)
                return 1
        
        elif args.command == "direct":
            result = workflow.merge_directly(
                args.target_asset,
                args.json_file
            )
            if result.get("success"):
                print("\nDirect merge completed successfully!")
                return 0
            else:
                print(f"\nMerge failed: {result.get('error', 'Unknown error')}", file=sys.stderr)
                return 1
    
    finally:
        workflow.cleanup()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
