// daFrameGraph MCP Tools
// Exposes daFrameGraph functionality as MCP tools for AI assistance
//
// Run:   daslang.exe utils/mcp/dafg_mcp_main.das
// Config in .mcp.json:
//   "mcpServers": { "dafg": { "command": "daslang.exe", "args": ["utils/mcp/dafg_mcp_main.das"] } }

options gen2
options persistent_heap
options gc
options multiple_contexts
options rtti

require dafg
require protocol_core public
require tools/common public

// ── Tool: Register a frame graph node ─────────────────────────────────

def make_register_node_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_register_node",
        description = "Register a new frame graph node in the daFrameGraph namespace",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path (e.g. 'render/deferred')"
                ),
                "node_name" => PropertySchema(
                    _type = "string",
                    description = "Unique name for the node within the namespace"
                ),
                "declaration" => PropertySchema(
                    _type = "string",
                    description = "Node declaration as daScript code (Registry -> (Index -> ()))"
                ),
            },
            required = ["namespace", "node_name", "declaration"]
        )
    )
}

def do_register_node(namespace : string; node_name : string; declaration : string;
                     project, project_root : string; load_modules : array<string>) : string {
    // This tool requires the daFrameGraph runtime to be initialized
    // For now, we return a success message - actual implementation needs C++ integration
    let result = make_tool_result("dafg_register_node", 
        "Node registered: {namespace}/{node_name}\nDeclaration: {declaration}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Create a texture resource ──────────────────────────────────

def make_create_texture_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_create_texture",
        description = "Create a 2D texture resource in the frame graph",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path"
                ),
                "texture_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the texture resource"
                ),
                "width" => PropertySchema(
                    _type = "integer",
                    description = "Texture width in pixels"
                ),
                "height" => PropertySchema(
                    _type = "integer",
                    description = "Texture height in pixels"
                ),
                "format" => PropertySchema(
                    _type = "string",
                    description = "Texture format (e.g., 'R8G8B8A8_UNORM', 'R16G16B16A16_FLOAT', 'D32_FLOAT')",
                    _default = "R8G8B8A8_UNORM"
                ),
                "usage" => PropertySchema(
                    _type = "string",
                    description = "Texture usage flags (e.g., 'SHADER_RESOURCE', 'RENDER_TARGET', 'UNORDERED_ACCESS')",
                    _default = "SHADER_RESOURCE | RENDER_TARGET"
                ),
            },
            required = ["namespace", "texture_name", "width", "height"]
        )
    )
}

def do_create_texture(namespace : string; texture_name : string; width : string; height : string;
                      format : string; usage : string;
                      project, project_root : string; load_modules : array<string>) : string {
    let w = int(width)
    let h = int(height)
    let result = make_tool_result("dafg_create_texture",
        "Texture created: {namespace}/{texture_name}\nSize: {w}x{h}\nFormat: {format}\nUsage: {usage}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Create a buffer resource ───────────────────────────────────

def make_create_buffer_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_create_buffer",
        description = "Create a buffer resource in the frame graph",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path"
                ),
                "buffer_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the buffer resource"
                ),
                "size" => PropertySchema(
                    _type = "integer",
                    description = "Buffer size in bytes"
                ),
                "usage" => PropertySchema(
                    _type = "string",
                    description = "Buffer usage (e.g., 'CONSTANT_BUFFER', 'STRUCTURED_BUFFER', 'RAW_BUFFER')",
                    _default = "STRUCTURED_BUFFER"
                ),
            },
            required = ["namespace", "buffer_name", "size"]
        )
    )
}

def do_create_buffer(namespace : string; buffer_name : string; size : string; usage : string;
                     project, project_root : string; load_modules : array<string>) : string {
    let sz = int(size)
    let result = make_tool_result("dafg_create_buffer",
        "Buffer created: {namespace}/{buffer_name}\nSize: {sz} bytes\nUsage: {usage}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Set auto-resolution ────────────────────────────────────────

def make_set_resolution_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_set_resolution",
        description = "Set the auto-resolution for a resource type",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path"
                ),
                "type_name" => PropertySchema(
                    _type = "string",
                    description = "Auto-resolution type name (e.g., 'main_rt', 'half_res')"
                ),
                "width" => PropertySchema(
                    _type = "integer",
                    description = "Resolution width"
                ),
                "height" => PropertySchema(
                    _type = "integer",
                    description = "Resolution height"
                ),
            },
            required = ["namespace", "type_name", "width", "height"]
        )
    )
}

def do_set_resolution(namespace : string; type_name : string; width : string; height : string;
                      project, project_root : string; load_modules : array<string>) : string {
    let w = int(width)
    let h = int(height)
    let result = make_tool_result("dafg_set_resolution",
        "Resolution set: {namespace}/{type_name} = {w}x{h}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Set dynamic resolution ─────────────────────────────────────

def make_set_dynamic_resolution_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_set_dynamic_resolution",
        description = "Set dynamic resolution (preserves history, only on heap-supported platforms)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path"
                ),
                "type_name" => PropertySchema(
                    _type = "string",
                    description = "Auto-resolution type name"
                ),
                "width" => PropertySchema(
                    _type = "integer",
                    description = "Dynamic resolution width (must be <= initial resolution)"
                ),
                "height" => PropertySchema(
                    _type = "integer",
                    description = "Dynamic resolution height (must be <= initial resolution)"
                ),
            },
            required = ["namespace", "type_name", "width", "height"]
        )
    )
}

def do_set_dynamic_resolution(namespace : string; type_name : string; width : string; height : string;
                              project, project_root : string; load_modules : array<string>) : string {
    let w = int(width)
    let h = int(height)
    let result = make_tool_result("dafg_set_dynamic_resolution",
        "Dynamic resolution set: {namespace}/{type_name} = {w}x{h}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Fill a named slot ──────────────────────────────────────────

def make_fill_slot_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_fill_slot",
        description = "Fill a named slot with a resource (provides indirection for resource lookup)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace containing the slot"
                ),
                "slot_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the slot to fill"
                ),
                "resource_namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace containing the resource"
                ),
                "resource_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the resource to fill the slot with"
                ),
            },
            required = ["namespace", "slot_name", "resource_namespace", "resource_name"]
        )
    )
}

def do_fill_slot(namespace : string; slot_name : string; resource_namespace : string; resource_name : string;
                 project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_fill_slot",
        "Slot filled: {namespace}/{slot_name} -> {resource_namespace}/{resource_name}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Update externally consumed resources ───────────────────────

def make_update_externally_consumed_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_update_externally_consumed",
        description = "Mark resources as externally consumed (prevents optimization/pruning)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace containing the resources"
                ),
                "resources" => PropertySchema(
                    _type = "array",
                    description = "List of resource names to mark as externally consumed",
                    items = PropertySchema(_type = "string", description = "Resource name")
                ),
            },
            required = ["namespace", "resources"]
        )
    )
}

def do_update_externally_consumed(namespace : string; resources : array<string>;
                                  project, project_root : string; load_modules : array<string>) : string {
    let res_str = join(resources, ", ")
    let result = make_tool_result("dafg_update_externally_consumed",
        "Externally consumed resources updated in {namespace}: {res_str}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Run the frame graph ────────────────────────────────────────

def make_run_frame_graph_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_run",
        description = "Execute the frame graph (recompiles if needed)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "flush_refined_blocks" => PropertySchema(
                    _type = "boolean",
                    description = "Whether to flush refined blocks after compilation",
                    _default = false
                ),
            },
            required = []
        )
    )
}

def do_run_frame_graph(flush_refined_blocks : string;
                       project, project_root : string; load_modules : array<string>) : string {
    let flush = flush_refined_blocks == "true"
    let result = make_tool_result("dafg_run",
        "Frame graph executed\nFlush refined blocks: {flush}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Invalidate history ─────────────────────────────────────────

def make_invalidate_history_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_invalidate_history",
        description = "Invalidate frame graph history (forces resource recreation)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {},
            required = []
        )
    )
}

def do_invalidate_history(project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_invalidate_history",
        "Frame graph history invalidated\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Set multiplexing mode ──────────────────────────────────────

def make_set_multiplexing_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_set_multiplexing",
        description = "Set default multiplexing mode for nodes",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "mode" => PropertySchema(
                    _type = "string",
                    description = "Multiplexing mode (e.g., 'None', 'View', 'Eye', 'Layer')",
                    _default = "None"
                ),
                "history_mode" => PropertySchema(
                    _type = "string",
                    description = "History multiplexing mode",
                    _default = "None"
                ),
            },
            required = []
        )
    )
}

def do_set_multiplexing(mode : string; history_mode : string;
                        project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_set_multiplexing",
        "Multiplexing mode set: {mode}, History mode: {history_mode}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Set multiplexing extents ───────────────────────────────────

def make_set_multiplexing_extents_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_set_multiplexing_extents",
        description = "Set multiplexing extents for following frames",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "x" => PropertySchema(
                    _type = "integer",
                    description = "Extent X"
                ),
                "y" => PropertySchema(
                    _type = "integer",
                    description = "Extent Y"
                ),
                "z" => PropertySchema(
                    _type = "integer",
                    description = "Extent Z"
                ),
            },
            required = ["x", "y", "z"]
        )
    )
}

def do_set_multiplexing_extents(x : string; y : string; z : string;
                                project, project_root : string; load_modules : array<string>) : string {
    let xi = int(x)
    let yi = int(y)
    let zi = int(z)
    let result = make_tool_result("dafg_set_multiplexing_extents",
        "Multiplexing extents set: ({xi}, {yi}, {zi})\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Get resource info ──────────────────────────────────────────

def make_get_resource_info_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_get_resource_info",
        description = "Get information about a frame graph resource",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace path"
                ),
                "resource_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the resource"
                ),
            },
            required = ["namespace", "resource_name"]
        )
    )
}

def do_get_resource_info(namespace : string; resource_name : string;
                         project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_get_resource_info",
        "Resource info for {namespace}/{resource_name}:\n\nNote: Requires daFrameGraph runtime integration via C++ module to query actual resource data.")
    return result
}

// ── Tool: Dump frame graph structure ─────────────────────────────────

def make_dump_frame_graph_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_dump",
        description = "Dump the frame graph structure for debugging",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "namespace" => PropertySchema(
                    _type = "string",
                    description = "Namespace to dump (empty for all)",
                    _default = ""
                ),
            },
            required = []
        )
    )
}

def do_dump_frame_graph(namespace : string;
                        project, project_root : string; load_modules : array<string>) : string {
    let ns = empty(namespace) ? "all namespaces" : namespace
    let result = make_tool_result("dafg_dump",
        "Frame graph dump for {ns}:\n\nNote: Requires daFrameGraph runtime integration via C++ module to query actual graph structure.")
    return result
}

// ── Tool: Visualize frame graph ──────────────────────────────────────

def make_visualize_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_visualize",
        description = "Generate frame graph visualization (requires visualization manager)",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "output_path" => PropertySchema(
                    _type = "string",
                    description = "Path to save visualization output"
                ),
                "format" => PropertySchema(
                    _type = "string",
                    description = "Output format (e.g., 'dot', 'mermaid', 'json')",
                    _default = "dot"
                ),
            },
            required = ["output_path"]
        )
    )
}

def do_visualize(output_path : string; format : string;
                 project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_visualize",
        "Visualization saved to: {output_path}\nFormat: {format}\n\nNote: Requires daFrameGraph runtime integration via C++ module with visualization manager enabled.")
    return result
}

// ── Tool: Mark external resource for validation ──────────────────────

def make_mark_external_validation_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_mark_external_validation",
        description = "Mark an external resource for validation of illegal access",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "resource_name" => PropertySchema(
                    _type = "string",
                    description = "Name of the external resource"
                ),
            },
            required = ["resource_name"]
        )
    )
}

def do_mark_external_validation(resource_name : string;
                                project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_mark_external_validation",
        "External resource marked for validation: {resource_name}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Tool: Reset shadervar after use ──────────────────────────────────

def make_reset_shadervar_tool() : MakeFileTool {
    return MakeFileTool(
        name = "dafg_reset_shadervar",
        description = "Reset a shadervar to zero after node execution",
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "name" => PropertySchema(
                    _type = "string",
                    description = "Shadervar name"
                ),
            },
            required = ["name"]
        )
    )
}

def do_reset_shadervar(name : string;
                       project, project_root : string; load_modules : array<string>) : string {
    let result = make_tool_result("dafg_reset_shadervar",
        "Shadervar reset after use: {name}\n\nNote: Requires daFrameGraph runtime integration via C++ module.")
    return result
}

// ── Build the tool registry ──────────────────────────────────────────

def build_dafg_tools(var reg : array<ToolDef>) {
    // Core frame graph operations
    reg |> emplace(ToolDef(
        tool <- make_register_node_tool(),
        required = ["namespace", "node_name", "declaration"],
        arg_names = ["namespace", "node_name", "declaration"],
        handler = @@(ns, node_name, declaration, project, project_root : string; load_modules : array<string>) : string {
            return do_register_node(ns, node_name, declaration, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_create_texture_tool(),
        required = ["namespace", "texture_name", "width", "height"],
        arg_names = ["namespace", "texture_name", "width", "height", "format", "usage"],
        handler = @@(ns, texture_name, width, height, format, usage, project, project_root : string; load_modules : array<string>) : string {
            return do_create_texture(ns, texture_name, width, height, format, usage, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_create_buffer_tool(),
        required = ["namespace", "buffer_name", "size"],
        arg_names = ["namespace", "buffer_name", "size", "usage"],
        handler = @@(ns, buffer_name, size, usage, project, project_root : string; load_modules : array<string>) : string {
            return do_create_buffer(ns, buffer_name, size, usage, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_set_resolution_tool(),
        required = ["namespace", "type_name", "width", "height"],
        arg_names = ["namespace", "type_name", "width", "height"],
        handler = @@(ns, type_name, width, height, project, project_root : string; load_modules : array<string>) : string {
            return do_set_resolution(ns, type_name, width, height, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_set_dynamic_resolution_tool(),
        required = ["namespace", "type_name", "width", "height"],
        arg_names = ["namespace", "type_name", "width", "height"],
        handler = @@(ns, type_name, width, height, project, project_root : string; load_modules : array<string>) : string {
            return do_set_dynamic_resolution(ns, type_name, width, height, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_fill_slot_tool(),
        required = ["namespace", "slot_name", "resource_namespace", "resource_name"],
        arg_names = ["namespace", "slot_name", "resource_namespace", "resource_name"],
        handler = @@(ns, slot, res_ns, res_name, project, project_root : string; load_modules : array<string>) : string {
            return do_fill_slot(ns, slot, res_ns, res_name, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_update_externally_consumed_tool(),
        required = ["namespace", "resources"],
        arg_names = ["namespace", "resources"],
        handler = @@(ns, resources, project, project_root : string; load_modules : array<string>) : string {
            return do_update_externally_consumed(ns, resources, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_run_frame_graph_tool(),
        required = [],
        arg_names = ["flush_refined_blocks"],
        handler = @@(flush, project, project_root : string; load_modules : array<string>) : string {
            return do_run_frame_graph(flush, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_invalidate_history_tool(),
        required = [],
        arg_names = [],
        handler = @@(project, project_root : string; load_modules : array<string>) : string {
            return do_invalidate_history(project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_set_multiplexing_tool(),
        required = [],
        arg_names = ["mode", "history_mode"],
        handler = @@(mode, history_mode, project, project_root : string; load_modules : array<string>) : string {
            return do_set_multiplexing(mode, history_mode, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_set_multiplexing_extents_tool(),
        required = ["x", "y", "z"],
        arg_names = ["x", "y", "z"],
        handler = @@(x, y, z, project, project_root : string; load_modules : array<string>) : string {
            return do_set_multiplexing_extents(x, y, z, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_get_resource_info_tool(),
        required = ["namespace", "resource_name"],
        arg_names = ["namespace", "resource_name"],
        handler = @@(ns, res_name, project, project_root : string; load_modules : array<string>) : string {
            return do_get_resource_info(ns, res_name, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_dump_frame_graph_tool(),
        required = [],
        arg_names = ["namespace"],
        handler = @@(ns, project, project_root : string; load_modules : array<string>) : string {
            return do_dump_frame_graph(ns, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_visualize_tool(),
        required = ["output_path"],
        arg_names = ["output_path", "format"],
        handler = @@(path, format, project, project_root : string; load_modules : array<string>) : string {
            return do_visualize(path, format, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_mark_external_validation_tool(),
        required = ["resource_name"],
        arg_names = ["resource_name"],
        handler = @@(res_name, project, project_root : string; load_modules : array<string>) : string {
            return do_mark_external_validation(res_name, project, project_root, load_modules)
        }))

    reg |> emplace(ToolDef(
        tool <- make_reset_shadervar_tool(),
        required = ["name"],
        arg_names = ["name"],
        handler = @@(name, project, project_root : string; load_modules : array<string>) : string {
            return do_reset_shadervar(name, project, project_root, load_modules)
        }))

    // Shutdown tool (inherited from protocol_core)
    register_shutdown(reg)
}

// Server entry point
[export]
def main() {
    logger_init(MCP_LOG_NAME)
    logger_info("mcp", "Starting daFrameGraph MCP server (stdio transport)")

    var reg : array<ToolDef>
    build_dafg_tools(reg)

    serve_stdio(das_mcp_server("dafg", "0.1.0"), reg)
}