# daslang MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/) (MCP) server that exposes 28 daslang compiler-backed tools to AI coding assistants like Claude Code — compilation diagnostics, program introspection, AOT generation, live-reload control, and more.

## Tools

### Compiler & Introspection

| Tool | Description |
|---|---|
| `compile_check` | Compile a `.das` file and return errors/warnings plus a categorized function listing on success. Optional `json` for structured output |
| `list_functions` | Compile a `.das` file and list all user functions, class methods, and generic instances (after macro expansion) |
| `list_types` | Compile a `.das` file and list all structs, classes (with fields), enums (with values), and type aliases |
| `run_test` | Run dastest on a `.das` test file and return pass/fail results. Optional `json` for structured output |
| `format_file` | Format a `.das` file using `daslib/das_source_formatter` |
| `run_script` | Run a `.das` file or inline code snippet and return stdout/stderr |
| `ast_dump` | Dump AST of an expression or compiled function. `mode=ast` returns S-expression (node types/fields), `mode=source` returns post-macro daslang code. Optional `lineinfo` to include file and line:col spans on each node |
| `program_log` | Produce full post-compilation program text (like `options log`). Shows all types, globals, and functions after macro expansion, template instantiation, and inference. Optional `function` filter |
| `list_modules` | List all available daslang modules (builtin C++ modules and daslib). Optional `json` for structured output |
| `find_symbol` | Cross-module symbol search (functions, generics, structs, handled types, enums, globals, typedefs/aliases, fields). Case-insensitive substring by default; `=query` for exact match |
| `list_requires` | Compile a `.das` file and list all `require` dependencies (direct and transitive), with source file paths and builtin annotations. Optional `json` for structured output |
| `list_module_api` | List all functions, types, enums, and globals exported by a builtin or daslib module (e.g. `math`, `strings`, `fio`, `daslib/json`). Optional `compact` mode for large modules |
| `convert_to_gen2` | Convert a `.das` file from gen1 (indentation-based) syntax to gen2 (braces/parentheses) using `das-fmt`. Optional `inplace` flag to modify the file directly |
| `goto_definition` | Given a cursor position (file, line, column), resolve the definition of the symbol under the cursor. Returns location, kind (variable/function/field/builtin/struct/enum/typedef), and source snippet. Optional `no_opt` to preserve pre-optimization AST |
| `type_of` | Given a cursor position (file, line, column), return the resolved type of the expression under the cursor. Shows all expressions at position from innermost to outermost. Optional `no_opt` |
| `find_references` | Find all references to the symbol under the cursor (function calls, variable uses, field accesses, type refs, enum/bitfield values, aliases). Works from both usage and declaration sites. Scope: `file` (default) or `all` (all loaded modules). Optional `no_opt` |
| `eval_expression` | Evaluate a daslang expression and return its printed result. Supports comma-separated module imports via `require` parameter |
| `describe_type` | Describe a type's fields, methods, values, and base type. Supports structs, classes, handled types, enums, bitfields, variants, tuples, typedefs |
| `grep_usage` | Parse-aware symbol search across `.das` files using ast-grep + tree-sitter. Finds identifier occurrences excluding comments and strings. Conditional on `sg` CLI |
| `outline` | List all declarations (functions, structs, classes, enums, bitfields, variants, globals, typedefs) in a file or set of files using tree-sitter. Works on broken/incomplete code — no compilation needed. Conditional on `sg` CLI |
| `aot` | Generate AOT (ahead-of-time) C++ code for a `.das` file or a single function. Without `function`, returns full AOT output. With `function`, extracts that function's C++ only. Overloaded names return a disambiguation list with mangled names for exact selection |

### Live-Reload Control

These tools interact with a running `daslang-live.exe` instance via its REST API (default port 9090). All accept an optional `port` parameter.

| Tool | Description |
|---|---|
| `live_launch` | Start a `daslang-live` instance with a script file. Sets working directory to the script's folder. Detects if already running. Polls up to 10 seconds to confirm startup |
| `live_status` | Get status (fps, uptime, paused, dt, has_error) |
| `live_error` | Get last compilation error (null if none) |
| `live_reload` | Trigger reload. Optional `full` param for full recompile. Works even during compilation errors |
| `live_pause` | Pause or unpause (`paused` = "true"/"false"). Returns 503 on compilation error |
| `live_command` | Dispatch a `[live_command]` (`name` required, optional `args` JSON string). Returns 503 on compilation error. Use `name="help"` to list all commands |
| `live_shutdown` | Graceful shutdown of the live instance |

### Server Management

| Tool | Description |
|---|---|
| `shutdown` | Shut down the MCP server itself. Claude Code auto-restarts it, picking up code changes to `.das` tool files. Tool registration changes require a manual restart |

## Setup

No extra build dependencies — the MCP server uses stdio transport. Claude Code manages the process lifecycle automatically.

```bash
# Manual test (Windows):
bin/Release/daslang.exe utils/mcp/main.das

# Manual test (Linux):
./bin/daslang utils/mcp/main.das
```

Configure in `.mcp.json` (project root):

```json
// Windows
{
  "mcpServers": {
    "daslang": {
      "command": "bin/Release/daslang.exe",
      "args": ["utils/mcp/main.das"]
    }
  }
}

// Linux
{
  "mcpServers": {
    "daslang": {
      "command": "./bin/daslang",
      "args": ["utils/mcp/main.das"]
    }
  }
}
```

Or add via CLI:

```bash
# Windows
claude mcp add daslang -- bin/Release/daslang.exe utils/mcp/main.das

# Linux
claude mcp add daslang -- ./bin/daslang utils/mcp/main.das
```

Claude Code starts and stops the server automatically with each session.

## Architecture

- Each tool invocation runs in a **separate thread** (`new_thread`) with its own context/heap — when the thread ends, its memory is freed without GC
- **Exception:** `live_*` tools run on the main thread (they use `system()` and `sleep()` which don't work well from `new_thread`)
- Protocol logic lives in `protocol.das`, the entry point is `main.das`
- Heap is collected after each request when over threshold (1 MB)
- Tool handlers are modular: each tool lives in `tools/*.das`, shared utilities in `tools/common.das`

## Configuring Claude Code

Optionally, allow the MCP tools without prompting by adding to `.claude/settings.json`:

```json
{
  "permissions": {
    "allow": [
      "mcp__daslang__compile_check",
      "mcp__daslang__list_functions",
      "mcp__daslang__list_types",
      "mcp__daslang__run_test",
      "mcp__daslang__format_file",
      "mcp__daslang__run_script",
      "mcp__daslang__ast_dump",
      "mcp__daslang__list_modules",
      "mcp__daslang__find_symbol",
      "mcp__daslang__list_requires",
      "mcp__daslang__list_module_api",
      "mcp__daslang__convert_to_gen2",
      "mcp__daslang__goto_definition",
      "mcp__daslang__type_of",
      "mcp__daslang__find_references",
      "mcp__daslang__program_log",
      "mcp__daslang__eval_expression",
      "mcp__daslang__describe_type",
      "mcp__daslang__grep_usage",
      "mcp__daslang__outline",
      "mcp__daslang__aot",
      "mcp__daslang__live_launch",
      "mcp__daslang__live_status",
      "mcp__daslang__live_error",
      "mcp__daslang__live_reload",
      "mcp__daslang__live_pause",
      "mcp__daslang__live_command",
      "mcp__daslang__live_shutdown",
      "mcp__daslang__shutdown"
    ]
  }
}
```

After creating/editing these files, restart Claude Code (or start a new session) for it to pick up the MCP server.

## ast-grep / tree-sitter setup

The `grep_usage` and `outline` tools use [ast-grep](https://ast-grep.github.io/) (`sg` CLI) with a custom tree-sitter grammar for daslang. The `sgconfig.yml` config file is platform-specific (shared library extension differs), so it is gitignored.

Copy the appropriate template to `sgconfig.yml` in the project root:

```bash
# Windows
cp sgconfig.yml.windows sgconfig.yml

# Linux
cp sgconfig.yml.linux sgconfig.yml

# macOS
cp sgconfig.yml.osx sgconfig.yml
```

## How It Works

The server implements the MCP protocol via JSON-RPC 2.0 over stdio, handling `initialize`, `tools/list`, `tools/call`, and `ping`.

- Reads newline-delimited JSON (NDJSON) from stdin
- Writes JSON-RPC responses to stdout (one line per message)
- Logs to stderr and to `utils/mcp/mcp_server.log`

File paths passed to tools are resolved relative to the server's working directory.
