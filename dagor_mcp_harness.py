#!/usr/bin/env python3
"""
Dagor Engine MCP Harness
Connects to the daFrameGraph MCP server and provides a Python interface
for AI assistants to interact with the Dagor Engine's frame graph system.
"""

import json
import subprocess
import threading
import queue
import os
import sys
import time
import signal
import atexit
from typing import Dict, Any, Optional, List, Union
from pathlib import Path
from dataclasses import dataclass, field


@dataclass
class MCPTool:
    """Represents an MCP tool definition"""
    name: str
    description: str
    input_schema: Dict[str, Any]


@dataclass
class MCPServerConfig:
    """Configuration for an MCP server"""
    name: str
    command: List[str]
    cwd: Optional[str] = None
    env: Optional[Dict[str, str]] = None


class MCPClient:
    """MCP Client for communicating with MCP servers over stdio"""
    
    def __init__(self, config: MCPServerConfig):
        self.config = config
        self.process: Optional[subprocess.Popen] = None
        self.request_id = 0
        self.pending_requests: Dict[int, threading.Event] = {}
        self.responses: Dict[int, Dict[str, Any]] = {}
        self.tools: List[MCPTool] = []
        self.initialized = False
        self._read_thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()
        
    def start(self) -> bool:
        """Start the MCP server process"""
        try:
            env = os.environ.copy()
            if self.config.env:
                env.update(self.config.env)
            
            self.process = subprocess.Popen(
                self.config.command,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=self.config.cwd,
                env=env,
                text=True,
                encoding='utf-8',
                errors='replace',
                bufsize=1  # Line buffered
            )
            
            # Start reader thread
            self._stop_event.clear()
            self._read_thread = threading.Thread(target=self._read_loop, daemon=True)
            self._read_thread.start()
            
            # Initialize
            return self._initialize()
            
        except Exception as e:
            print(f"Failed to start MCP server: {e}")
            return False
    
    def _initialize(self) -> bool:
        """Perform MCP initialization handshake"""
        init_request = {
            "jsonrpc": "2.0",
            "id": self._next_id(),
            "method": "initialize",
            "params": {
                "protocolVersion": "2025-11-25",
                "capabilities": {},
                "clientInfo": {"name": "dagor-mcp-harness", "version": "0.1.0"}
            }
        }
        
        response = self._send_request(init_request, timeout=10)
        if response and "result" in response:
            self.initialized = True
            
            # Send initialized notification
            self._send_notification({
                "jsonrpc": "2.0",
                "method": "notifications/initialized"
            })
            
            # List available tools
            self._refresh_tools()
            return True
        return False
    
    def _refresh_tools(self):
        """Fetch available tools from server"""
        response = self._send_request({
            "jsonrpc": "2.0",
            "id": self._next_id(),
            "method": "tools/list"
        }, timeout=5)
        
        if response and "result" in response:
            self.tools = [
                MCPTool(
                    name=tool["name"],
                    description=tool.get("description", ""),
                    input_schema=tool.get("inputSchema", {})
                )
                for tool in response["result"].get("tools", [])
            ]
            print(f"Loaded {len(self.tools)} tools from {self.config.name}")
    
    def _next_id(self) -> int:
        self.request_id += 1
        return self.request_id
    
    def _send_request(self, request: Dict[str, Any], timeout: float = 30) -> Optional[Dict[str, Any]]:
        """Send a request and wait for response"""
        if not self.process or self.process.poll() is not None:
            raise RuntimeError("MCP server not running")
        
        request_id = request.get("id")
        if request_id is None:
            raise ValueError("Request must have an id")
        
        event = threading.Event()
        self.pending_requests[request_id] = event
        
        try:
            # Write request
            self.process.stdin.write(json.dumps(request, separators=(',', ':')) + '\n')
            self.process.stdin.flush()
            
            # Wait for response
            if event.wait(timeout=timeout):
                return self.responses.pop(request_id, None)
            else:
                raise TimeoutError(f"Request {request_id} timed out")
        finally:
            self.pending_requests.pop(request_id, None)
    
    def _send_notification(self, notification: Dict[str, Any]):
        """Send a notification (no response expected)"""
        if not self.process or self.process.poll() is not None:
            raise RuntimeError("MCP server not running")
        
        self.process.stdin.write(json.dumps(notification, separators=(',', ':')) + '\n')
        self.process.stdin.flush()
    
    def call_tool(self, name: str, arguments: Dict[str, Any], timeout: float = 60) -> Optional[Dict[str, Any]]:
        """Call an MCP tool"""
        request = {
            "jsonrpc": "2.0",
            "id": self._next_id(),
            "method": "tools/call",
            "params": {
                "name": name,
                "arguments": arguments
            }
        }
        return self._send_request(request, timeout)
    
    def _read_loop(self):
        """Background thread to read responses from server"""
        while not self._stop_event.is_set() and self.process and self.process.poll() is None:
            try:
                line = self.process.stdout.readline()
                if not line:
                    break
                
                line = line.strip()
                if not line:
                    continue
                
                try:
                    response = json.loads(line)
                except json.JSONDecodeError:
                    continue
                
                # Handle response
                if "id" in response:
                    request_id = response["id"]
                    self.responses[request_id] = response
                    if request_id in self.pending_requests:
                        self.pending_requests[request_id].set()
                        
            except Exception:
                break
    
    def stop(self):
        """Stop the MCP server"""
        self._stop_event.set()
        
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            except Exception:
                pass
        
        if self._read_thread:
            self._read_thread.join(timeout=2)
    
    def __enter__(self):
        self.start()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()


class DagorMCPHarness:
    """High-level harness for interacting with Dagor Engine via MCP"""
    
    def __init__(self, engine_root: str = "D:\\GaijinEngine"):
        self.engine_root = Path(engine_root)
        self.daslang_mcp: Optional[MCPClient] = None
        self.dafg_mcp: Optional[MCPClient] = None
        
    def start(self) -> bool:
        """Start all MCP servers"""
        success = True
        
        # Start daScript MCP server (for general daslang operations)
        daslang_config = MCPServerConfig(
            name="daslang",
            command=["daslang.exe", "utils/mcp/main.das"],
            cwd=str(self.engine_root)
        )
        
        self.daslang_mcp = MCPClient(daslang_config)
        if not self.daslang_mcp.start():
            print("Warning: Failed to start daslang MCP server")
            success = False
        
        # Start daFrameGraph MCP server
        dafg_config = MCPServerConfig(
            name="dafg",
            command=["daslang.exe", "utils/mcp/dafg_mcp_main.das"],
            cwd=str(self.engine_root)
        )
        
        self.dafg_mcp = MCPClient(dafg_config)
        if not self.dafg_mcp.start():
            print("Warning: Failed to start daFrameGraph MCP server")
            success = False
        
        return success
    
    def stop(self):
        """Stop all MCP servers"""
        if self.daslang_mcp:
            self.daslang_mcp.stop()
        if self.dafg_mcp:
            self.dafg_mcp.stop()
    
    # daFrameGraph Tools
    
    def register_node(self, namespace: str, node_name: str, declaration: str) -> str:
        """Register a frame graph node"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_register_node", {
            "namespace": namespace,
            "node_name": node_name,
            "declaration": declaration
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def create_texture(self, namespace: str, texture_name: str, width: int, height: int,
                       format: str = "R8G8B8A8_UNORM", usage: str = "SHADER_RESOURCE | RENDER_TARGET") -> str:
        """Create a 2D texture resource"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_create_texture", {
            "namespace": namespace,
            "texture_name": texture_name,
            "width": str(width),
            "height": str(height),
            "format": format,
            "usage": usage
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def create_buffer(self, namespace: str, buffer_name: str, size: int,
                      usage: str = "STRUCTURED_BUFFER") -> str:
        """Create a buffer resource"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_create_buffer", {
            "namespace": namespace,
            "buffer_name": buffer_name,
            "size": str(size),
            "usage": usage
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def set_resolution(self, namespace: str, type_name: str, width: int, height: int) -> str:
        """Set auto-resolution for a resource type"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_set_resolution", {
            "namespace": namespace,
            "type_name": type_name,
            "width": str(width),
            "height": str(height)
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def set_dynamic_resolution(self, namespace: str, type_name: str, width: int, height: int) -> str:
        """Set dynamic resolution (preserves history)"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_set_dynamic_resolution", {
            "namespace": namespace,
            "type_name": type_name,
            "width": str(width),
            "height": str(height)
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def fill_slot(self, namespace: str, slot_name: str, resource_namespace: str, resource_name: str) -> str:
        """Fill a named slot with a resource"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_fill_slot", {
            "namespace": namespace,
            "slot_name": slot_name,
            "resource_namespace": resource_namespace,
            "resource_name": resource_name
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def update_externally_consumed(self, namespace: str, resources: List[str]) -> str:
        """Mark resources as externally consumed"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_update_externally_consumed", {
            "namespace": namespace,
            "resources": resources
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def run_frame_graph(self, flush_refined_blocks: bool = False) -> str:
        """Execute the frame graph"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_run", {
            "flush_refined_blocks": str(flush_refined_blocks).lower()
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def invalidate_history(self) -> str:
        """Invalidate frame graph history"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_invalidate_history", {})
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def set_multiplexing(self, mode: str = "None", history_mode: str = "None") -> str:
        """Set multiplexing mode"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_set_multiplexing", {
            "mode": mode,
            "history_mode": history_mode
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def set_multiplexing_extents(self, x: int, y: int, z: int) -> str:
        """Set multiplexing extents"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_set_multiplexing_extents", {
            "x": str(x),
            "y": str(y),
            "z": str(z)
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def get_resource_info(self, namespace: str, resource_name: str) -> str:
        """Get information about a resource"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_get_resource_info", {
            "namespace": namespace,
            "resource_name": resource_name
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def dump_frame_graph(self, namespace: str = "") -> str:
        """Dump frame graph structure"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_dump", {
            "namespace": namespace
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def visualize(self, output_path: str, format: str = "dot") -> str:
        """Generate frame graph visualization"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_visualize", {
            "output_path": output_path,
            "format": format
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def mark_external_validation(self, resource_name: str) -> str:
        """Mark external resource for validation"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_mark_external_validation", {
            "resource_name": resource_name
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def reset_shadervar(self, name: str) -> str:
        """Reset shadervar after use"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_reset_shadervar", {
            "name": name
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    # Voxel Rendering Tools
    
    def register_voxel_node(self, namespace: str, node_name: str, voxel_type: str = "opaque") -> str:
        """Register a voxel rendering node in the frame graph"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        declaration = f"voxel_{voxel_type}"
        result = self.dafg_mcp.call_tool("dafg_register_node", {
            "namespace": namespace,
            "node_name": node_name,
            "declaration": declaration
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def create_voxel_texture_array(self, namespace: str, texture_name: str, width: int, height: int, layers: int,
                                   format: str = "R8G8B8A8_UNORM") -> str:
        """Create a texture array for voxel atlas"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_create_texture", {
            "namespace": namespace,
            "name": texture_name,
            "width": str(width),
            "height": str(height),
            "format": format
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def create_vertex_buffer(self, namespace: str, buffer_name: str, vertex_count: int, 
                             stride: int, usage: str = "VERTEX_BUFFER") -> str:
        """Create a vertex buffer for voxel meshes"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        size = vertex_count * stride
        result = self.dafg_mcp.call_tool("dafg_create_buffer", {
            "namespace": namespace,
            "name": buffer_name,
            "size": str(size),
            "usage": usage
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def create_index_buffer(self, namespace: str, buffer_name: str, index_count: int) -> str:
        """Create an index buffer for voxel meshes"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        size = index_count * 4  # 4 bytes per uint32
        result = self.dafg_mcp.call_tool("dafg_create_buffer", {
            "namespace": namespace,
            "name": buffer_name,
            "size": str(size),
            "usage": "INDEX_BUFFER"
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def set_voxel_resolution(self, namespace: str, width: int, height: int, layers: int) -> str:
        """Set resolution for voxel texture arrays"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        result = self.dafg_mcp.call_tool("dafg_set_resolution", {
            "namespace": namespace,
            "resource": "voxel_atlas",
            "width": str(width),
            "height": str(height)
        })
        
        if result and "result" in result:
            return result["result"].get("content", [{}])[0].get("text", "Success")
        return f"Error: {result}"
    
    def setup_voxel_pipeline(self, namespace: str = "voxel") -> str:
        """Set up a complete voxel rendering pipeline"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        # Create voxel namespace
        self.dafg_mcp.call_tool("dafg_namespace", {"path": namespace})
        
        # Create voxel atlas texture array
        self.create_voxel_texture_array(namespace, "voxel_atlas", 1024, 1024, 256)
        
        # Create vertex/index buffers for chunk meshes
        self.create_vertex_buffer(namespace, "chunk_vertices", 1000000, 32)
        self.create_index_buffer(namespace, "chunk_indices", 3000000)
        
        # Create uniform buffer for voxel shader constants
        self.create_buffer(namespace, "voxel_constants", 256, "CONSTANT_BUFFER")
        
        # Register voxel rendering nodes
        self.register_voxel_node(namespace, "voxel_opaque", "opaque")
        self.register_voxel_node(namespace, "voxel_transparent", "transparent")
        self.register_voxel_node(namespace, "voxel_water", "water")
        
        # Set resolution for voxel resources
        self.set_resolution(namespace, "voxel_atlas", 1024, 1024)
        
        return "Voxel pipeline set up in namespace: " + namespace
    
    def update_voxel_buffers(self, namespace: str, vertex_data: str, index_data: str) -> str:
        """Update voxel mesh buffers with new data (placeholder for runtime updates)"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        return "Buffer update would be implemented via frame graph resource updates"
    
    def dump_voxel_pipeline(self, namespace: str = "voxel") -> str:
        """Dump voxel pipeline frame graph"""
        if not self.dafg_mcp:
            return "daFrameGraph MCP not connected"
        
        return self.dump_frame_graph(namespace)
    
    def list_tools(self) -> List[MCPTool]:
        """List all available tools from daFrameGraph MCP"""
        if not self.dafg_mcp:
            return []
        return self.dafg_mcp.tools
    
    def __enter__(self):
        self.start()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()


# Example usage
def main():
    print("Dagor Engine MCP Harness")
    print("=" * 50)
    
    harness = DagorMCPHarness()
    
    try:
        if not harness.start():
            print("Failed to start MCP servers")
            return 1
        
        print("\nAvailable daFrameGraph tools:")
        for tool in harness.list_tools():
            print(f"  - {tool.name}: {tool.description}")
        
        print("\n--- Testing daFrameGraph operations ---")
        
        # Example: Create a texture
        print("\n1. Creating a texture:")
        result = harness.create_texture("render/deferred", "gbuffer_normal", 1920, 1080, "R16G16B16A16_FLOAT", "RENDER_TARGET | SHADER_RESOURCE")
        print(result)
        
        # Example: Set resolution
        print("\n2. Setting resolution:")
        result = harness.set_resolution("render", "main_rt", 1920, 1080)
        print(result)
        
        # Example: Fill a slot
        print("\n3. Filling a slot:")
        result = harness.fill_slot("render", "gbuffer_depth", "render", "depth_buffer")
        print(result)
        
        # Example: Run frame graph
        print("\n4. Running frame graph:")
        result = harness.run_frame_graph()
        print(result)
        
        # Example: Dump frame graph
        print("\n5. Dumping frame graph:")
        result = harness.dump_frame_graph("render")
        print(result)
        
    finally:
        harness.stop()
        print("\nMCP servers stopped")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())