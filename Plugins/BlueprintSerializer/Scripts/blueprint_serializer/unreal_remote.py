#!/usr/bin/env python3
"""
Unreal Remote Execution Module

Provides communication with Unreal Editor via the Remote Execution plugin
or commandlet execution for Blueprint operations.
"""

import socket
import json
import subprocess
import os
from pathlib import Path
from typing import Optional, Dict, Any, List


class UnrealRemote:
    """
    Interface for communicating with Unreal Editor.
    
    Supports two modes:
    1. Remote Execution Plugin (TCP socket) - for live editor communication
    2. Commandlet Execution - for batch processing without editor
    """
    
    DEFAULT_HOST = "127.0.0.1"
    DEFAULT_PORT = 6776  # Unreal Remote Execution port
    
    def __init__(
        self,
        host: str = DEFAULT_HOST,
        port: int = DEFAULT_PORT,
        project_path: Optional[str] = None,
        engine_path: Optional[str] = None
    ):
        """
        Initialize Unreal Remote connection.
        
        Args:
            host: Remote execution host address
            port: Remote execution port
            project_path: Path to .uproject file
            engine_path: Path to Unreal Engine installation
        """
        self.host = host
        self.port = port
        self.project_path = project_path
        self.engine_path = engine_path
        self._socket: Optional[socket.socket] = None
        
    def connect(self) -> bool:
        """
        Attempt to connect to the Unreal Editor remote execution server.
        
        Returns:
            True if connection successful, False otherwise
        """
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(5.0)
            self._socket.connect((self.host, self.port))
            return True
        except (socket.error, socket.timeout) as e:
            print(f"Failed to connect to Unreal Editor: {e}")
            self._socket = None
            return False
    
    def disconnect(self):
        """Close the connection to Unreal Editor."""
        if self._socket:
            try:
                self._socket.close()
            except:
                pass
            self._socket = None
    
    def is_connected(self) -> bool:
        """Check if connected to Unreal Editor."""
        return self._socket is not None
    
    def execute_python(self, code: str) -> Dict[str, Any]:
        """
        Execute Python code in Unreal Editor.
        
        Args:
            code: Python code to execute
            
        Returns:
            Dictionary with 'success' and 'result' or 'error' keys
        """
        if not self.is_connected():
            if not self.connect():
                return {"success": False, "error": "Not connected to Unreal Editor"}
        
        try:
            # Format the remote execution message
            message = {
                "type": "execute_python",
                "code": code
            }
            
            # Send message
            data = json.dumps(message).encode('utf-8')
            self._socket.sendall(len(data).to_bytes(4, 'little') + data)
            
            # Receive response
            response_len = int.from_bytes(self._socket.recv(4), 'little')
            response_data = self._socket.recv(response_len)
            response = json.loads(response_data.decode('utf-8'))
            
            return response
            
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def export_blueprint(self, asset_path: str, output_path: str) -> Dict[str, Any]:
        """
        Export a Blueprint to JSON file.
        
        Args:
            asset_path: Unreal asset path (e.g., '/Game/Blueprints/BP_MyActor')
            output_path: Output JSON file path
            
        Returns:
            Dictionary with operation result
        """
        code = f'''
import unreal

# Load the Blueprint
blueprint = unreal.EditorAssetLibrary.load_asset("{asset_path}")
if blueprint is None:
    result = {{"success": False, "error": "Blueprint not found: {asset_path}"}}
else:
    # Get the BlueprintSerializer module
    serializer = unreal.get_editor_subsystem(unreal.BlueprintSerializerSubsystem)
    if serializer:
        json_string = serializer.serialize_blueprint_to_string(blueprint)
        with open(r"{output_path}", "w", encoding="utf-8") as f:
            f.write(json_string)
        result = {{"success": True, "output_path": r"{output_path}"}}
    else:
        # Fallback: Use JsonObjectGraph experimental feature
        options = unreal.JsonStringifyOptions()
        filename = ""
        unreal.JsonObjectGraphFunctionLibrary.write_blueprint_class_to_temp_file(
            blueprint, "export", options, filename
        )
        result = {{"success": True, "output_path": filename, "method": "JsonObjectGraph"}}
'''
        return self.execute_python(code)
    
    def import_blueprint(
        self,
        json_path: str,
        package_path: str,
        blueprint_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Import a Blueprint from JSON file.
        
        Args:
            json_path: Path to JSON file
            package_path: Unreal package path for the new Blueprint
            blueprint_name: Optional name for the Blueprint
            
        Returns:
            Dictionary with operation result
        """
        name_param = f'"{blueprint_name}"' if blueprint_name else '""'
        
        code = f'''
import unreal

# Read the JSON file
with open(r"{json_path}", "r", encoding="utf-8") as f:
    json_string = f.read()

# Get the BlueprintSerializer module
serializer = unreal.get_editor_subsystem(unreal.BlueprintSerializerSubsystem)
if serializer:
    blueprint = serializer.deserialize_blueprint_from_string(
        json_string, "{package_path}", {name_param}
    )
    if blueprint:
        result = {{"success": True, "asset_path": blueprint.get_path_name()}}
    else:
        result = {{"success": False, "error": "Failed to deserialize Blueprint"}}
else:
    result = {{"success": False, "error": "BlueprintSerializer subsystem not available"}}
'''
        return self.execute_python(code)

    def merge_json_into_blueprint(
        self,
        json_path: str,
        target_asset_path: str
    ) -> Dict[str, Any]:
        """
        Merge JSON file into an existing Blueprint (adds new nodes without removing existing ones).
        
        Args:
            json_path: Path to JSON file with new nodes/variables
            target_asset_path: Unreal asset path of the Blueprint to merge into
            
        Returns:
            Dictionary with operation result
        """
        code = f'''
import unreal

# Load the target Blueprint
blueprint = unreal.EditorAssetLibrary.load_asset("{target_asset_path}")
if blueprint is None:
    result = {{"success": False, "error": "Blueprint not found: {target_asset_path}"}}
else:
    # Read the JSON file
    with open(r"{json_path}", "r", encoding="utf-8") as f:
        json_string = f.read()
    
    # Get the BlueprintSerializer module
    serializer = unreal.get_editor_subsystem(unreal.BlueprintSerializerSubsystem)
    if serializer:
        success = serializer.merge_json_string_into_blueprint(blueprint, json_string)
        if success:
            # Save the Blueprint
            unreal.EditorAssetLibrary.save_asset(blueprint.get_path_name())
            result = {{"success": True, "asset_path": blueprint.get_path_name()}}
        else:
            result = {{"success": False, "error": "Failed to merge JSON into Blueprint"}}
    else:
        result = {{"success": False, "error": "BlueprintSerializer subsystem not available"}}
'''
        return self.execute_python(code)
    
    def open_merge_editor(
        self,
        original_asset_path: str,
        modified_asset_path: str
    ) -> Dict[str, Any]:
        """
        Open the Blueprint merge editor.
        
        Args:
            original_asset_path: Path to original Blueprint
            modified_asset_path: Path to modified Blueprint
            
        Returns:
            Dictionary with operation result
        """
        code = f'''
import unreal

original_bp = unreal.EditorAssetLibrary.load_asset("{original_asset_path}")
modified_bp = unreal.EditorAssetLibrary.load_asset("{modified_asset_path}")

if original_bp is None:
    result = {{"success": False, "error": "Original Blueprint not found"}}
elif modified_bp is None:
    result = {{"success": False, "error": "Modified Blueprint not found"}}
else:
    # Use the merge helper
    merge_helper = unreal.BlueprintMergeHelper
    success = merge_helper.open_merge_editor(original_bp, modified_bp)
    result = {{"success": success}}
'''
        return self.execute_python(code)
    
    def run_commandlet(self, commandlet_name: str, args: List[str]) -> int:
        """
        Run an Unreal commandlet for batch processing.
        
        Args:
            commandlet_name: Name of the commandlet
            args: Additional arguments
            
        Returns:
            Process return code
        """
        if not self.project_path or not self.engine_path:
            raise ValueError("project_path and engine_path required for commandlet execution")
        
        editor_cmd = Path(self.engine_path) / "Engine" / "Binaries" / "Win64" / "UnrealEditor-Cmd.exe"
        
        cmd = [
            str(editor_cmd),
            self.project_path,
            f"-run={commandlet_name}",
            *args
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Commandlet error: {result.stderr}")
        
        return result.returncode


def get_default_remote() -> UnrealRemote:
    """
    Get a default UnrealRemote instance configured from environment.
    
    Environment variables:
        UNREAL_PROJECT_PATH: Path to .uproject file
        UNREAL_ENGINE_PATH: Path to Unreal Engine installation
        UNREAL_REMOTE_HOST: Remote execution host (default: 127.0.0.1)
        UNREAL_REMOTE_PORT: Remote execution port (default: 6776)
    """
    return UnrealRemote(
        host=os.environ.get("UNREAL_REMOTE_HOST", UnrealRemote.DEFAULT_HOST),
        port=int(os.environ.get("UNREAL_REMOTE_PORT", str(UnrealRemote.DEFAULT_PORT))),
        project_path=os.environ.get("UNREAL_PROJECT_PATH"),
        engine_path=os.environ.get("UNREAL_ENGINE_PATH"),
    )
