# Blueprint Serializer Python Scripts

Python automation tools for the Blueprint Serializer plugin, enabling Copilot MCP integration for Blueprint manipulation workflows.

## Installation

```bash
pip install -e .
```

Or use directly:
```bash
cd Plugins/BlueprintSerializer/Scripts
python -m blueprint_serializer
```

## Prerequisites

1. **Unreal Editor Running**: The editor must be running with the Remote Execution plugin enabled
2. **Blueprint Serializer Plugin**: The plugin must be enabled in your project

## Usage

### Export Blueprint to JSON

Export a Blueprint asset to AI-readable JSON format:

```bash
python -m blueprint_serializer.export_blueprint /Game/Blueprints/BP_MyActor -o BP_MyActor.json
```

### Import Blueprint from JSON

Create a new Blueprint from a JSON file:

```bash
python -m blueprint_serializer.import_blueprint BP_MyActor.json /Game/Blueprints -n BP_MyActor_New
```

### Compare Two Blueprints

Show differences between two Blueprint JSON files:

```bash
python -m blueprint_serializer.diff_blueprints original.json modified.json
```

### AI-Assisted Workflow

#### Step 1: Prepare for AI

Export a Blueprint and get instructions for AI modification:

```bash
python -m blueprint_serializer.merge_workflow prepare /Game/Blueprints/BP_MyActor
```

#### Step 2: Apply AI Modifications

Modify the exported JSON file using AI (Copilot, etc.)

#### Step 3: Merge Changes

Import the modified JSON and open the merge editor:

```bash
python -m blueprint_serializer.merge_workflow merge /Game/Blueprints/BP_MyActor BP_MyActor_modified.json
```

## Environment Variables

- `UNREAL_PROJECT_PATH`: Path to your .uproject file
- `UNREAL_ENGINE_PATH`: Path to Unreal Engine installation
- `UNREAL_REMOTE_HOST`: Remote execution host (default: 127.0.0.1)
- `UNREAL_REMOTE_PORT`: Remote execution port (default: 6776)

## API Usage

```python
from blueprint_serializer import export_blueprint, import_blueprint, diff_blueprints
from blueprint_serializer.merge_workflow import MergeWorkflow

# Export
result = export_blueprint("/Game/Blueprints/BP_MyActor", "output.json")

# Import
result = import_blueprint("modified.json", "/Game/Blueprints", "BP_New")

# Diff
result = diff_blueprints("original.json", "modified.json")
print(result["summary"])

# Full workflow
workflow = MergeWorkflow()
result = workflow.run_full_workflow(
    "/Game/Blueprints/BP_MyActor",
    "modified.json",
    auto_open_merge=True
)
```
