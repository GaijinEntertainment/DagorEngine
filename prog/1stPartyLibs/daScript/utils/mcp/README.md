# daslang MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/) (MCP) server that exposes daslang compiler-backed tools to AI coding assistants like Claude Code - compilation diagnostics, program introspection, AOT generation, C++ source intelligence, live-reload control, and more.

## Tools

### Compiler & Introspection

| Tool | Description |
|---|---|
| `compile_check` | Compile a `.das` file and return errors/warnings plus a categorized function listing on success. Optional `json` for structured output |
| `list_functions` | Compile a `.das` file and list all user functions, class methods, and generic instances (after macro expansion) |
| `list_types` | Compile a `.das` file and list all structs, classes (with fields), enums (with values), and type aliases |
| `run_test` | Run dastest on a `.das` test file and return pass/fail results. Optional `json` for structured output |
| `format_file` | Format a `.das` file using `daslib/das_source_formatter` |
| `run_script` | Run a `.das` file or inline code snippet and return stdout/stderr. Optional `project` for `.das_project`-bound module resolution. |
| `ast_dump` | Dump AST of an expression or compiled function. `mode=ast` returns S-expression (node types/fields), `mode=source` returns post-macro daslang code. Optional `lineinfo` to include file and line:col spans on each node |
| `program_log` | Produce full post-compilation program text (like `options log`). Shows all types, globals, and functions after macro expansion, template instantiation, and inference. Optional `function` filter |
| `list_modules` | List all available daslang modules in three sections: the C++ modules the process registered, the daslib files, and the require paths descriptors register (`module/path`). With `project_root` or `load_modules` the listing comes from a child that scanned those descriptors too, eager. Optional `json` for structured output |
| `find_symbol` | Cross-module symbol search (functions, generics, structs, handled types, enums, globals, typedefs/aliases, fields). Case-insensitive substring by default; `=query` for exact match |
| `discover` | Intent search over the generated module digest (`skills/daslang/references/everything.md`): every public symbol with its one-line description. An exact name match ranks first; then rows matching more of the query's words; within equal coverage the score decides, and a word weighs most in the symbol name, then the group heading, then the description, through a small synonym list. FTS5 syntax (`uri*`, `"file name"`, AND/OR/NOT, NEAR) filters first and ranks the survivors. Optional `limit` (default 12), `module` |
| `list_requires` | Compile a `.das` file and list all `require` dependencies (direct and transitive), with source file paths and builtin annotations. Optional `json` for structured output |
| `list_module_api` | List all functions, types, enums, and globals exported by a builtin or daslib module (e.g. `math`, `strings`, `fio`, `daslib/json`). Optional `compact` mode for large modules |
| `convert_to_gen2` | Convert a `.das` file from gen1 (indentation-based) syntax to gen2 (braces/parentheses) using the `utils/gen1-to-gen2` converter. Optional `inplace` flag to modify the file directly |
| `goto_definition` | Given a cursor position (file, line, column), resolve the definition of the symbol under the cursor. Returns location, kind (variable/function/field/builtin/struct/enum/typedef), and source snippet. Optional `no_opt` to preserve pre-optimization AST |
| `type_of` | Given a cursor position (file, line, column), return the resolved type of the expression under the cursor. Shows all expressions at position from innermost to outermost. Optional `no_opt` |
| `find_references` | Find all references to the symbol under the cursor (function calls, variable uses, field accesses, type refs, enum/bitfield values, aliases). Works from both usage and declaration sites. Scope: `file` (default) or `all` (all loaded modules). Optional `no_opt` |
| `eval_expression` | Evaluate a daslang expression and return its printed result. Supports comma-separated module imports via `require` parameter. Optional `project` for `.das_project`-bound module resolution. |
| `describe_type` | Describe a type's fields, methods, values, and base type. Supports structs, classes, handled types, enums, bitfields, variants, tuples, typedefs |
| `grep_usage` | Parse-aware symbol search across `.das` files using ast-grep + tree-sitter. Finds identifier occurrences excluding comments and strings. Conditional on `sg` CLI |
| `outline` | List all declarations (functions, structs, classes, enums, bitfields, variants, globals, typedefs) in a file or set of files using tree-sitter. Works on broken/incomplete code - no compilation needed. Conditional on `sg` CLI |
| `aot` | Generate AOT (ahead-of-time) C++ code for a `.das` file or a single function. Without `function`, returns full AOT output. With `function`, extracts that function's C++ only. Overloaded names return a disambiguation list with mangled names for exact selection |
| `arch_of` | The `[arch]` citations a `.das` file makes, each resolved to its document plus the section text under the cited anchor heading. Optional `symbol` to keep only one declaration's citations. Unresolved citations report LINT026's own reason. Reads source, so a file that does not compile still answers |
| `arch_sites` | The census of an architecture document: every `.das` under the folder that owns it citing it, with file, line and annotated symbol. Grouped under the document's own headings (uncited anchors included) unless the query names an anchor |

### C++ Source & Build Tools

Parse-aware (tree-sitter-cpp) source search plus compiler-backed build tools. The compile tools read the CMake compile database (`build/compile_commands.json`); see the note below.

| Tool | Description |
|---|---|
| `cpp_grep_usage` | Parse-aware C++ identifier search across `.cpp/.h/.hpp/.cc` files using ast-grep + tree-sitter-cpp. Skips comments and strings. Searches the C++ index roots by default (`src/`, `include/`, `modules/` in an in-tree session) |
| `cpp_find_symbol` | Search C++ symbol DECLARATIONS by name + kind (`function`/`class`/`struct`/`enum`/`union`/`typedef`/`namespace`/`macro`). Best-effort; macro-expanded declarations are invisible to ast-grep |
| `cpp_outline` | List C++ declarations in a file or glob, grouped by file with containment (methods under their class). Works on broken code; no compile DB needed |
| `cpp_goto_definition` | Up to 5 plausible definition locations for a cursor position. Approximate - no scope resolution or overload disambiguation |
| `cpp_compile_check` | Syntax-check a C++ translation unit using the real compiler off `compile_commands.json` (`cl /Zs` on MSVC, `-fsyntax-only` on clang/gcc). Inherits the build's flags incl. `/WX`. Optional `json` for a structured `CppCompileResult`; optional `build_dir` |
| `cpp_build_info` | Return the compiler, build directory, full compile command, and derived syntax-only command for a TU. Answers "what command line compiles this file" |
| `cpp_format_file` | Format a C++ file in place with clang-format, but only when a `.clang-format` is discoverable by walking up from the file. No-op-with-message otherwise (the daScript tree ships none) |

**Compile DB requirement.** `cpp_compile_check` / `cpp_build_info` need `build/compile_commands.json`. The top-level `CMakeLists.txt` sets `CMAKE_EXPORT_COMPILE_COMMANDS ON`, but only the **Ninja and Makefile** generators honor it - the **Visual Studio generator does not emit the DB**, so on Windows use a side Ninja build dir (the tools probe `build/`, `build-ninja/`, then `build*/` under the served tree when it is a project, the daslang root otherwise; pass `build_dir` to override). Headers aren't translation units (not in the DB) - pass a `.cpp`/`.cc` that includes them.

**Windows + MSVC: developer environment.** On MSVC the DB omits system include paths (the compiler reads them from the `INCLUDE` env var set by `vcvars64`), so the MCP server - and the `cl.exe` it spawns - must run in a Visual Studio developer environment, or `cpp_compile_check` fails on `<vcruntime.h>`. The simplest fix is to point `.mcp.json` at the bundled launcher `utils/mcp/daslang-mcp-msvc.cmd`, which locates VS via `vswhere`, loads the x64 dev environment, then starts the server - so it works no matter how Claude Code is launched:

```json
"daslang": {
  "command": "cmd",
  "args": ["/c", "utils\\mcp\\daslang-mcp-msvc.cmd"],
  "defer_loading": false
}
```

Alternatively, launch Claude Code itself from an *x64 Native Tools Command Prompt for VS* (the server inherits the environment). clang/gcc find their system headers automatically, so this is Windows/MSVC-only - on Linux/macOS point `.mcp.json` straight at the daslang binary.

#### C++ index roots {#cpp-index-roots}

The `cpp_*` tools scan `CPP_SEARCH_DIRS` (`cpp_search_config.das` - `src`, `include`, `modules`)
under the daslang root. A server serving another tree scans that tree whole instead, when the tree
is a project: a `.git`, `CMakeLists.txt`, `.mcp.json` or `compile_commands.json` at its root
(`CPP_PROJECT_MARKERS`, `tools/common.das`). The configured folders are daslang's own layout and
say nothing about another project, and a single root keeps the index and every result path relative
to the tree the caller asked about. A served tree with no marker keeps the daslang layout: a server
started in a home directory or at `/` (the standalone bundle's smoke test runs there) would
otherwise index everything under it.

### Two servers: full (`main.das`) and C++-only (`cpp_main.das`)

The server has two entry points over the same dispatch core (the provider-neutral `mcp_core.das`, with the daslang module-resolution layer added by `protocol_core.das`):

- **`main.das`** - the full tool set (everything above).
- **`cpp_main.das`** - only the cpp/agnostic subset: `grep_usage`, `outline`, the seven `cpp_*` tools, and `shutdown`. None of the daslang compiler-backed tools (compile / lint / AOT / introspection / live-reload) are registered, so a C++-only project gets a focused tool list without the daslang toolchain.

Register one or both. `utils/mcp/setup.das` writes the `daslang` entry as the watchdog's stdio front over the tree's own binary - the front answers `initialize` itself, spawns the server on the first tool call, and respawns it after a kill or a rebuild, so a session never loses its tools (`utils/watchdog/README.md`, "Serving a stdio client"):

```json
"mcpServers": {
  "daslang": { "command": "<tree>/bin/watchdog",
               "args": ["--stdio", "--name", "daslang-mcp", "--cwd", "<tree>",
                        "--program", "<tree>/bin/daslang", "--", "-ignore-manifest", "utils/mcp/main.das"] }
}
```

On **Windows** the child is the vcvars launcher, so `cpp_compile_check` finds `cl.exe`: `"--program", "C:\\Windows\\System32\\cmd.exe", "--", "/c", "<tree>\\utils\\mcp\\daslang-mcp-msvc.cmd"`; the launcher's first argument selects the server script, so the C++-only server is the same line with `cpp_main.das` appended. A tree without the watchdog built runs the front through the interpreter: `"command"` is the binary and `"args"` start with `"utils/watchdog/main.das", "--"`.

The bare form still works, minus the respawn - point the entry at the binary directly:

```json
"mcpServers": {
  "daslang":     { "command": "./bin/daslang", "args": ["-ignore-manifest", "utils/mcp/main.das"] },
  "daslang-cpp": { "command": "./bin/daslang", "args": ["-ignore-manifest", "utils/mcp/cpp_main.das"] }
}
```

Either way the server needs `-ignore-manifest`: without it the server enumerates only the modules a compile loaded, so `list_modules` and the all-modules symbol scans come up short.

Tools are namespaced by server, so the cpp server's tools appear as `mcp__daslang-cpp__cpp_compile_check` etc. `cpp-mcp` - a standalone static AOT build of `cpp_main.das` for C++-only consumers - exists as a gated target (`DAS_BUILD_CPP_MCP`, OFF by default; bundled by `ci/make_cpp_mcp_bundle.sh`, released via `cpp_mcp_release.yml`, setup in `cpp-mcp-setup.md`); the interpreted form above is the same server. It is a separate product: the mcp server itself never ships as a `daslang -exe` binary - development runs it interpreted under the watchdog front, so that exe form would never be dogfooded; `utils/REVIEW.das` refuses the target until the front's `--program` names the exe.

### Duplicate Detection

| Tool | Description |
|---|---|
| `export_corpus` | Scan one or more `.das` files / directories / globs and write a corpus JSON to `out`. Same shape as `detect-dupe --export-functions` (subprocess wrapper around `utils/detect-dupe/main.das`) |
| `detect_duplicates` | Compare candidate file(s) against a pre-built corpus. Returns an envelope with the per-candidate JSON report (top-N exact and fuzzy matches per candidate). Supports `keep` to override the default pattern filter |
| `judge_duplicates` | AI judge: take a `detect-dupe` JSON report and ask Claude to partition each cluster into real / partial / false_positive verdicts. Shells out to `daslang utils/find-dupe/main.das` - requires `daspkg install --root utils/find-dupe` first (the `anthropic/anthropic` package) and `ANTHROPIC_API_KEY`. WARNING: sends source to Anthropic's API |
| `find_dupe` | Convenience: run `detect-dupe` against `paths` and judge the resulting clusters in one call. Same daspkg + API-key requirement as `judge_duplicates` |

### Live-Reload Control

These tools interact with a running `daslang-live.exe` instance via its REST API (default port 9090). All accept an optional `port` parameter.

| Tool | Description |
|---|---|
| `live_launch` | Start a `daslang-live` instance with a script file. Sets working directory to the script's folder. Detects if already running. Polls up to 10 seconds to confirm startup. Optional `project` is forwarded as `-project <file>` for `.das_project`-bound module resolution |
| `live_status` | Get status (fps, uptime, paused, dt, has_error) |
| `live_error` | Get last compilation error (null if none) |
| `live_reload` | Trigger reload. Optional `full` param for full recompile. Works even during compilation errors |
| `live_pause` | Pause or unpause (`paused` = "true"/"false"). Returns 503 on compilation error |
| `live_command` | Dispatch a `[live_command]` (`name` required, optional `args` JSON string). Returns 503 on compilation error. Use `name="help"` to list all commands |
| `live_commands` | Dispatch a batch of `[live_command]`s in one round-trip; continue-on-error semantics, response is a JSON array preserving input order |
| `live_shutdown` | Graceful shutdown of the live instance |

### Server Management

| Tool | Description |
|---|---|
| `shutdown` | Shut down the MCP server itself. Claude Code auto-restarts it, picking up code changes to `.das` tool files. Tool registration changes require a manual restart |

## Setup

No extra build dependencies - the MCP server uses stdio transport. Claude Code manages the process lifecycle automatically.

```bash
# Manual test (Windows):
bin/Release/daslang.exe -ignore-manifest utils/mcp/main.das

# Manual test (Linux):
./bin/daslang -ignore-manifest utils/mcp/main.das
```

Configure in `.mcp.json` (project root):

```json
// Windows
{
  "mcpServers": {
    "daslang": {
      "command": "bin/Release/daslang.exe",
      "args": ["-ignore-manifest", "utils/mcp/main.das"]
    }
  }
}

// Linux
{
  "mcpServers": {
    "daslang": {
      "command": "./bin/daslang",
      "args": ["-ignore-manifest", "utils/mcp/main.das"]
    }
  }
}
```

Or add via CLI:

```bash
# Windows
claude mcp add daslang -- bin/Release/daslang.exe -ignore-manifest utils/mcp/main.das

# Linux
claude mcp add daslang -- ./bin/daslang -ignore-manifest utils/mcp/main.das
```

Claude Code starts and stops the server automatically with each session.

### Fresh checkouts and worktrees

A new `git worktree add` (or a fresh clone) does **not** carry the files the MCP
server needs: `.mcp.json`, `sgconfig.yml`, a built `bin/` binary, and the
tree-sitter grammar shared lib are all gitignored, so a Claude session opened in
a worktree silently has zero daslang tools. Bootstrap one with:

```bash
# run from ANY existing daslang binary, pointed at the target worktree
bin/Release/daslang.exe utils/mcp/setup.das -- --root <worktree-abs-path>   # Windows MSVC
./bin/daslang           utils/mcp/setup.das -- --root <worktree-abs-path>   # Linux / macOS
daslang                 utils/mcp/setup.das -- --root <worktree-abs-path>   # when daslang is on PATH
```

`setup.das` builds a worktree-local `daslang` (+ the `tree_sitter_daslang`
grammar), copies the platform `sgconfig.yml` template, and writes/merges
`.mcp.json` with the `daslang` server entry and the `daslang-dap` entry (the
DAP bridge, `utils/dap/main.das`, under the same front). The LSP needs no
entry: its plugin manifest is checked in. It adds no new secrets, and
preserves any existing server entries (e.g. `github`, including their env
blocks) as-is. Pass `--no-build` to only wire the
config to an already-built binary. On Windows it also points the build at a
shared OpenSSL cache (`%LOCALAPPDATA%/daslang/openssl`; override with
`--openssl-dir`) so OpenSSL is built once instead of ~15 min per worktree.
Restart the session in the worktree afterward to pick up the server.

> **`das_root` is derived from the binary, not the cwd** (`src/misc/sysos.cpp`):
> daslang resolves its root (daslib, modules) from the directory above `bin/`.
> `setup.das` therefore points `.mcp.json` at the **worktree-local** binary so
> file resolution stays inside the worktree. If you ever wire a *shared* binary
> from another checkout, pass `-dasroot <worktree>` in `args` or every tool will
> read the wrong tree. A tool's relative path is another matter - it resolves against
> the served tree (see "The served tree" under How It Works).

## Architecture

- Each tool invocation runs in a **separate thread** (`new_thread`) with its own context/heap - when the thread ends, its memory is freed without GC
- **Exception:** `live_*` tools run on the main thread (they use `system()` and `sleep()` which don't work well from `new_thread`)
- Dispatch + JSON-RPC framing live in the provider-neutral `mcp_core.das` (serverInfo and the extra-arg names it extracts for every tool come from the `McpServer` config it is handed); `protocol_core.das` adds the daslang layer - the module-resolution args (`project` / `project_root` / `load_modules`), `make_file_tool`, and `das_mcp_server()`. Tools are described by a data-driven registry (`array<ToolDef>`): `registry_das.das` registers the daslang compiler-backed tools, `registry_cpp.das` the cpp/agnostic subset. Adding a tool = one `ToolDef` entry.
- Two entry points share that dispatch: **`main.das`** registers the full set; **`cpp_main.das`** registers only the cpp/agnostic subset (ast-grep search/outline + the C++ build tools + `shutdown`), for C++-focused consumers that don't want the daslang toolchain in their tool list.
- Heap is collected after each request when over threshold (1 MB)
- Tool handlers are modular: each tool lives in `tools/*.das`, MCP-specific shared utilities in `tools/common.das`. The general "comma/newline list of files / globs -> file array" expander (`expand_glob`, `parse_file_list`) lives in `daslib/fio`

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
      "mcp__daslang__cpp_grep_usage",
      "mcp__daslang__cpp_find_symbol",
      "mcp__daslang__cpp_outline",
      "mcp__daslang__cpp_goto_definition",
      "mcp__daslang__cpp_compile_check",
      "mcp__daslang__cpp_build_info",
      "mcp__daslang__cpp_format_file",
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

### The served tree {#served-tree}

A tool's relative path resolves against the server's working directory - the tree the stdio front
entered with `--cwd`. `server_root()` (`tools/common.das`) names that tree; it is spelled generic,
with forward slashes, like `get_das_root()`, so a result compares against it as a string. In an
in-tree session the served tree is the daslang root; when another project's `.mcp.json` names this
checkout's watchdog it is that project's root. `das_root` is separate: daslang derives it from the
directory above `bin/`, so module resolution stays in the checkout that built the server, whatever
tree is served.
