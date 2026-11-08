// daFrameGraph MCP Server Main Entry Point
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
require mcp_core public

// Create the MCP server config
def create_dafg_server() : McpServer {
    return das_mcp_server("daFrameGraph", "1.0.0")
}

// Register a simple tool that returns API info
def register_api_info_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_get_api_info",
            "Get daFrameGraph API version and supported types",
            {
                "detail" => PropertySchema(_type = "string", description = "What info to return: 'version', 'nodes', 'resources', 'access', 'usage'")
            },
            ["detail"]
        ),
        required = ["detail"],
        arg_names = ["detail"],
        handler = @@(detail, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            var result = ""
            if (detail == "version" || empty(detail)) {
                result = get_api_version()
            } elif (detail == "nodes") {
                let types = get_node_types()
                result = "Node types:\n"
                for (t in types) {
                    result += "  - {t}\n"
                }
            } elif (detail == "resources") {
                let types = get_resource_types()
                result = "Resource types:\n"
                for (t in types) {
                    result += "  - {t}\n"
                }
            } elif (detail == "access") {
                let modes = get_access_modes()
                result = "Access modes:\n"
                for (m in modes) {
                    result += "  - {m}\n"
                }
            } elif (detail == "usage") {
                let types = get_usage_types()
                result = "Usage types:\n"
                for (t in types) {
                    result += "  - {t}\n"
                }
            } else {
                result = "Unknown detail: {detail}. Use: version, nodes, resources, access, usage"
            }
            return make_tool_result(result)
        }
    ))
}

// Register a tool for creating a namespace path
def register_namespace_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_namespace",
            "Create or navigate daFrameGraph namespace",
            {
                "path" => PropertySchema(_type = "string", description = "Namespace path (e.g., 'render/deferred/lighting')")
            },
            ["path"]
        ),
        required = ["path"],
        arg_names = ["path"],
        handler = @@(path, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            var ns = root_namespace()
            let parts = split(path, "/")
            for (p in parts) {
                if (!empty(p)) {
                    ns = ns / p
                }
            }
            return make_tool_result("Namespace created: {path} (nameSpaceId = {ns.nameSpaceId})")
        }
    ))
}

// Register a tool for registering a node
def register_node_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_register_node",
            "Register a frame graph node in a namespace",
            {
                "namespace" => PropertySchema(_type = "string", description = "Namespace path (e.g., 'render/deferred')"),
                "name" => PropertySchema(_type = "string", description = "Node name"),
                "source" => PropertySchema(_type = "string", description = "Node source code or type identifier")
            },
            ["namespace", "name", "source"]
        ),
        required = ["namespace", "name", "source"],
        arg_names = ["namespace", "name", "source"],
        handler = @@(namespace, name, source, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            var ns = root_namespace()
            let parts = split(namespace, "/")
            for (p in parts) {
                if (!empty(p)) {
                    ns = ns / p
                }
            }
            let handle = ns.register_node(name, source)
            return make_tool_result("Node registered: {namespace}/{name} -> handle id = {handle.id}")
        }
    ))
}

// Register a tool for creating texture resource
def register_create_texture_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_create_texture",
            "Create a texture resource",
            {
                "namespace" => PropertySchema(_type = "string", description = "Namespace path"),
                "name" => PropertySchema(_type = "string", description = "Texture name"),
                "width" => PropertySchema(_type = "string", description = "Width in pixels"),
                "height" => PropertySchema(_type = "string", description = "Height in pixels"),
                "format" => PropertySchema(_type = "string", description = "Format (e.g., 'R8G8B8A8_UNORM')")
            },
            ["namespace", "name", "width", "height"]
        ),
        required = ["namespace", "name", "width", "height"],
        arg_names = ["namespace", "name", "width", "height", "format"],
        handler = @@(namespace, name, width, height, format, arg6, project, project_root : string; load_modules : array<string>) : string {
            var ns = root_namespace()
            let parts = split(namespace, "/")
            for (p in parts) {
                if (!empty(p)) {
                    ns = ns / p
                }
            }
            let w = int(width)
            let h = int(height)
            let handle = ns.register_node("create_texture", "texture({name}, {w}, {h}, {format})")
            return make_tool_result("Texture created: {namespace}/{name} ({w}x{h}) format={format}")
        }
    ))
}

// Register a tool for creating buffer resource
def register_create_buffer_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_create_buffer",
            "Create a buffer resource",
            {
                "namespace" => PropertySchema(_type = "string", description = "Namespace path"),
                "name" => PropertySchema(_type = "string", description = "Buffer name"),
                "size" => PropertySchema(_type = "string", description = "Size in bytes"),
                "usage" => PropertySchema(_type = "string", description = "Usage flags (e.g., 'STRUCTURED_BUFFER')")
            },
            ["namespace", "name", "size"]
        ),
        required = ["namespace", "name", "size"],
        arg_names = ["namespace", "name", "size", "usage"],
        handler = @@(namespace, name, size, usage, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            var ns = root_namespace()
            let parts = split(namespace, "/")
            for (p in parts) {
                if (!empty(p)) {
                    ns = ns / p
                }
            }
            let sz = int(size)
            let handle = ns.register_node("create_buffer", "buffer({name}, {sz}, {usage})")
            return make_tool_result("Buffer created: {namespace}/{name} ({sz} bytes) usage={usage}")
        }
    ))
}

// Register a tool for setting resolution
def register_set_resolution_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_set_resolution",
            "Set explicit resolution for a resource",
            {
                "namespace" => PropertySchema(_type = "string", description = "Namespace path"),
                "resource" => PropertySchema(_type = "string", description = "Resource name"),
                "width" => PropertySchema(_type = "string", description = "Width"),
                "height" => PropertySchema(_type = "string", description = "Height")
            },
            ["namespace", "resource", "width", "height"]
        ),
        required = ["namespace", "resource", "width", "height"],
        arg_names = ["namespace", "resource", "width", "height"],
        handler = @@(namespace, resource, width, height, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            let w = int(width)
            let h = int(height)
            return make_tool_result("Resolution set for {namespace}/{resource}: {w}x{h}")
        }
    ))
}

// Register a tool for running the frame graph
def register_run_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_run",
            "Execute the frame graph",
            {
                "flush" => PropertySchema(_type = "string", description = "Flush refined blocks: 'true' or 'false'")
            },
            []
        ),
        required = array<string>(),
        arg_names = ["flush"],
        handler = @@(flush, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            return make_tool_result("Frame graph executed (flush={flush})")
        }
    ))
}

// Register a tool for dumping frame graph
def register_dump_tool(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "dafg_dump",
            "Dump frame graph for debugging",
            {
                "namespace" => PropertySchema(_type = "string", description = "Namespace path (empty for root)")
            },
            []
        ),
        required = array<string>(),
        arg_names = ["namespace"],
        handler = @@(namespace, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            var reg_ptr = get_registry()
            let dump = dump_frame_graph(reg_ptr)
            return make_tool_result(dump)
        }
    ))
}

// Build the tool registry
def build_registry() : array<ToolDef> {
    var reg : array<ToolDef>
    
    // Register shutdown tool (from protocol_core)
    register_shutdown(reg)
    
    // Register daFrameGraph tools
    register_api_info_tool(reg)
    register_namespace_tool(reg)
    register_node_tool(reg)
    register_create_texture_tool(reg)
    register_create_buffer_tool(reg)
    register_set_resolution_tool(reg)
    register_run_tool(reg)
    register_dump_tool(reg)
    
    return <- reg
}

[export]
def main() {
    logger_init(MCP_LOG_NAME)
    logger_info("mcp", "Starting daFrameGraph MCP server (stdio transport)")
    
    let server = create_dafg_server()
    let reg = build_registry()
    
    serve_stdio(server, reg)
}