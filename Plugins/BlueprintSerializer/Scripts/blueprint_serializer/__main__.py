#!/usr/bin/env python3
"""
Blueprint Serializer CLI

Main entry point for the blueprint_serializer package.
"""

import argparse
import sys


def main():
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        prog="blueprint_serializer",
        description="Blueprint Serializer - AI-friendly Blueprint manipulation tools"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Export command
    subparsers.add_parser(
        "export",
        help="Export Blueprint to JSON (run: python -m blueprint_serializer.export_blueprint)"
    )
    
    # Import command  
    subparsers.add_parser(
        "import",
        help="Import Blueprint from JSON (run: python -m blueprint_serializer.import_blueprint)"
    )
    
    # Diff command
    subparsers.add_parser(
        "diff",
        help="Compare two Blueprint JSONs (run: python -m blueprint_serializer.diff_blueprints)"
    )
    
    # Graph editor commands
    subparsers.add_parser(
        "graph",
        help="Blueprint Graph Editor - AI-friendly Blueprint manipulation tools (run: python -m blueprint_serializer.graph_cli)"
    )
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        print("\n\nExamples:")
        print("  # Export a Blueprint to JSON")
        print("  python -m blueprint_serializer.export_blueprint /Game/Blueprints/BP_MyActor -o BP_MyActor.json")
        print("")
        print("  # Import a Blueprint from JSON")
        print("  python -m blueprint_serializer.import_blueprint BP_MyActor.json /Game/Blueprints -n BP_MyActor_New")
        print("")
        print("  # Compare two Blueprint JSONs")
        print("  python -m blueprint_serializer.diff_blueprints original.json modified.json")
        print("")
        print("  # Prepare Blueprint for AI modification")
        print("  python -m blueprint_serializer.merge_workflow prepare /Game/Blueprints/BP_MyActor")
        print("")
        print("  # Merge AI modifications back")
        print("  python -m blueprint_serializer.merge_workflow merge /Game/Blueprints/BP_MyActor modified.json")
        return 0
    
    # Redirect to appropriate module
    if args.command == "export":
        from .export_blueprint import main as export_main
        return export_main()
    elif args.command == "import":
        from .import_blueprint import main as import_main
        return import_main()
    elif args.command == "diff":
        from .diff_blueprints import main as diff_main
        return diff_main()
    elif args.command == "merge":
        from .merge_workflow import main as merge_main
        return merge_main()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
