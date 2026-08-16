// daslang MCP layer over the provider-neutral core (mcp_core).
//
// Adds the two das-compiler concerns the neutral core deliberately leaves out:
// the module-resolution arguments (project / project_root / load_modules) —
// their advertised schema plus the make_file_tool helper that injects them —
// and das_mcp_server(), which wires those arg names into the McpServer config
// so the core extracts them for every tool. It re-exports mcp_core publicly, so
// registry_das / registry_cpp / main / cpp_main require only this module and see
// the full surface (make_tool, ToolDef, serve_stdio, …) unchanged.

options gen2
options rtti

module protocol_core

require mcp_core public
require tools/common public

// ── Module-resolution argument schemas (das-compiler specific) ────────

let PROJECT_PROP = PropertySchema(_type = "string", description = "Path to a .das_project file for custom module resolution")
let PROJECT_ROOT_PROP = PropertySchema(_type = "string", description = "Project root directory (parent of modules/) for daspkg-style module resolution; equivalent to daslang's -project_root CLI flag. Use when working on an external module via a <DummyRoot>/modules/<your-module> junction.")
let LOAD_MODULES_PROP = PropertySchema(
    _type = "array",
    description = "List of individual module folders (each containing .das_module) to load directly, equivalent to daslang's -load_module CLI flag (repeatable). Each path is the module's own folder — no <project_root>/modules/<name> wrapper. Path basenames shadow same-named entries in dasroot and project_root. Use to work on an external module without the junction trick.",
    items = new PropertySchema(_type = "string", description = "Path to a module folder (the one containing .das_module). Absolute paths are recommended; live_launch absolutizes them before spawning daslang-live so its -cwd flag doesn't shift the base, but other tools forward verbatim.")
)

def make_file_tool(name, description : string) : MakeFileTool {
    return MakeFileTool(
        name = name,
        description = description,
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "file" => PropertySchema(
                    _type = "string",
                    description = "Path to the .das file"
                ),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            required = ["file"]
        )
    )
}

// The daslang server config: serverInfo plus the module-resolution args the
// core extracts for every tool into the handler's extra1 / extra2 / extra_list
// slots (project, project_root, load_modules).
def das_mcp_server(name, version : string) : McpServer {
    return McpServer(
        info = ServerInfo(name = name, version = version),
        str_extra_args <- ["project", "project_root"],
        arr_extra_arg = "load_modules"
    )
}

// Annotate every tool result with the cross-tree guard: a CROSS-TREE WARNING
// when `file` points outside the server's own tree, whose answer used the
// wrong tree's module sources + compiled-in bindings and may be stale.
// Installed as the neutral core's result filter so mcp_core stays neutral.
[init] def install_crosstree_guard() {
    install_tool_result_filter(@@(result : string; args : JsonValue?) : string {
        return guard_crosstree(result, get_string_arg(args, "file"), get_string_arg(args, "project_root"))
    })
}

// shutdown is shared by every entry point (main + cpp_main). It runs on the
// main thread and exits the process before the response is written.
def register_shutdown(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "shutdown",
            "Shut down the MCP server. Claude Code will auto-restart it, picking up any code changes to MCP tools. Tool registration changes still require a manual restart.",
            { "unused" => PropertySchema(_type = "string", description = "ignored") },
            []),
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) : string {
            unsafe { exit(0); }
            return make_tool_result("shutting down")
        }))
}
