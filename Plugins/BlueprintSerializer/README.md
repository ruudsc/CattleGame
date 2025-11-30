# Blueprint Serializer Plugin

An Unreal Engine 5.6 editor plugin that converts Blueprint assets to/from AI-readable JSON format, enabling programmatic analysis and modification of Blueprints with AI assistance.

## Features

- **Export Blueprints to JSON** - Convert any Blueprint to a structured, AI-readable JSON format
- **Import Blueprints from JSON** - Create new Blueprint assets from JSON definitions
- **Three-way Merge** - Use UE5's native IMerge module to merge AI modifications with original Blueprints
- **Python Automation** - CLI tools for Copilot/AI integration

## Installation

The plugin is already included in the project. Enable it in Edit → Plugins → Blueprint Serializer.

## Usage

### From Editor

1. **Export**: Right-click a Blueprint in Content Browser → Blueprint Serializer → Export to JSON
2. **Import/Merge**: Right-click a Blueprint → Blueprint Serializer → Import from JSON... (merges new nodes into existing Blueprint)

### From Python (Copilot MCP Integration)

```bash
cd Plugins/BlueprintSerializer/Scripts
pip install -e .

# Export Blueprint
python -m blueprint_serializer.export_blueprint /Game/Blueprints/BP_MyActor -o BP_MyActor.json

# Import Blueprint (creates new asset)
python -m blueprint_serializer.import_blueprint BP_MyActor.json /Game/Blueprints

# Direct Merge (adds new nodes to existing Blueprint)
python -m blueprint_serializer.merge_workflow direct /Game/Blueprints/BP_MyActor modified.json

# Full AI workflow (creates new BP + opens merge editor)
python -m blueprint_serializer.merge_workflow prepare /Game/Blueprints/BP_MyActor
# ... AI modifies the JSON ...
python -m blueprint_serializer.merge_workflow merge /Game/Blueprints/BP_MyActor modified.json

# Diff two Blueprints
python -m blueprint_serializer.diff_blueprints original.json modified.json
```

## AI-Assisted Workflow

1. **Export** - User exports Blueprint to JSON via Content Browser or Python CLI
2. **AI Analysis** - AI (Copilot) reads JSON and generates modifications
3. **Create New Blueprint** - Modified JSON is imported as a new Blueprint asset
4. **Merge** - IMerge editor opens for three-way merge (original ↔ AI-modified)
5. **Review & Accept** - User reviews changes and accepts/rejects modifications

## JSON Schema

The JSON format includes:
- **Metadata**: Blueprint name, path, type, parent class, versions
- **Variables**: Name, type, category, default value, replication settings
- **Event Graphs**: Nodes with positions, pins, connections
- **Functions**: Signature, parameters, implementation graph
- **Components**: Hierarchy, class, properties
- **Interfaces**: Implemented interfaces
- **Dependencies**: Hard and soft asset references

Example:
```json
{
  "Metadata": {
    "BlueprintName": "BP_MyActor",
    "BlueprintPath": "/Game/Blueprints/BP_MyActor",
    "BlueprintType": "Normal",
    "ParentClass": "/Script/Engine.Actor",
    "SerializerVersion": "1.0.0"
  },
  "Variables": [...],
  "EventGraphs": [...],
  "Functions": [...],
  "Components": [...]
}
```

## Module Structure

```
Plugins/BlueprintSerializer/
├── BlueprintSerializer.uplugin
├── Source/
│   ├── BlueprintSerializer/           # Runtime module (JSON format definitions)
│   │   ├── Public/
│   │   │   ├── BlueprintSerializer.h
│   │   │   └── BlueprintJsonFormat.h  # JSON schema structs
│   │   └── Private/
│   └── BlueprintSerializerEditor/     # Editor module (serialization logic)
│       ├── Public/
│       │   ├── BlueprintToJson.h      # Serialization
│       │   ├── JsonToBlueprint.h      # Deserialization
│       │   └── BlueprintMergeHelper.h # Merge utilities
│       └── Private/
└── Scripts/                           # Python automation
    └── blueprint_serializer/
```

## Dependencies

- Core, CoreUObject, Engine, Json, JsonUtilities
- UnrealEd, BlueprintGraph, Kismet (Editor)
- Merge module for three-way merge functionality
