# cpp-mcp — C++ source-intelligence MCP server (setup)

`cpp-mcp` is a standalone [MCP](https://modelcontextprotocol.io) server exposing
daslang's **C++ source tooling** — parse-aware search/outline (via ast-grep) and
compile-DB-driven syntax checks — without pulling in the full daslang toolchain.
It's the curated cpp/agnostic subset of the daslang MCP server, shipped as a
self-contained AOT binary.

It is **language/project agnostic**: point it at any C/C++ codebase.

## What's in the bundle

```
cpp-mcp/
  bin/cpp-mcp[.exe]            # the server (statically linked; AOT-compiled hot paths)
  utils/mcp/*.das              # server sources (compiled at startup)
  utils/mcp/tools/*.das
  utils/common/git_signature.das
  utils/mcp/daslang-mcp-msvc.cmd   # Windows vcvars launcher
  tree-sitter-daslang/*.yml    # ast-grep rule files (loaded at runtime)
  daslib/                      # daslang standard library (sources)
  README.md                    # this file
```

The binary recompiles `cpp_main.das` (+ `daslib`) at startup — AOT swaps in the
native code, but the sources must be present. `getDasRoot()` derives the source
root from the executable's path, so **keep `cpp-mcp` in `bin/` next to the
bundled `utils/` and `daslib/`** (or pass `--root <bundle-dir>`).

## Install

1. Download the bundle for your platform from the
   [releases page](https://github.com/GaijinEntertainment/daScript/releases) and
   extract it anywhere (e.g. `~/tools/cpp-mcp`).
2. Add a server entry to your MCP client's `.mcp.json`:

   **Linux / macOS** — point straight at the binary (clang/gcc find system
   headers on their own):
   ```json
   {
     "mcpServers": {
       "cpp-mcp": {
         "command": "/abs/path/to/cpp-mcp/bin/cpp-mcp",
         "defer_loading": false
       }
     }
   }
   ```

   **Windows (MSVC)** — launch through the bundled `.cmd` so `cl.exe` finds the
   system headers (`compile_commands.json` omits them on MSVC; the compiler reads
   them from the `INCLUDE` env that vcvars sets):
   ```json
   {
     "mcpServers": {
       "cpp-mcp": {
         "command": "cmd",
         "args": ["/c", "C:\\abs\\path\\to\\cpp-mcp\\utils\\mcp\\daslang-mcp-msvc.cmd", "cpp-mcp.exe"],
         "defer_loading": false
       }
     }
   }
   ```

3. Restart your MCP client and call **`cpp_status`** — it reports exactly which
   dependencies are present and which tools that enables. When checking the
   compile DB for an external project, pass `build_dir` (your project's build
   directory); without it, `cpp_status` probes the bundle root, not your project.

## Tools

| Tool | Needs |
|---|---|
| `cpp_status` | nothing — self-diagnosis of the items below |
| `cpp_compile_check` | a `compile_commands.json` (CMake `CMAKE_EXPORT_COMPILE_COMMANDS=ON`); on MSVC, a vcvars env |
| `cpp_build_info` | a `compile_commands.json` |
| `cpp_format_file` | `clang-format` on PATH + a `.clang-format` in the project |
| `cpp_grep_usage`, `cpp_find_symbol`, `cpp_outline`, `cpp_goto_definition` | [ast-grep](https://ast-grep.github.io) (`sg`) on PATH |
| `grep_usage`, `outline` | ast-grep — for `.das` files |

The compile-DB and status tools work out of the box. The six ast-grep tools are
**enabled only if `sg` is on PATH** (run `cpp_status` to confirm); they degrade
gracefully when it isn't.

### Optional: enable the ast-grep tools

Install [ast-grep](https://ast-grep.github.io/guide/quick-start.html) and ensure
`sg` is on PATH. The bundle already ships the rule files
(`tree-sitter-daslang/*.yml`), so the **C/C++** ast-grep tools (`cpp_grep_usage`,
`cpp_find_symbol`, `cpp_outline`, `cpp_goto_definition`) work with just `sg`
installed. The `.das` tools (`grep_usage`, `outline`) additionally need the
tree-sitter-daslang grammar + an `sgconfig.yml`.

## Notes

- `--root <dir>` overrides source-root autodetection (use when running the
  binary from outside its bundle layout).
- The server speaks JSON-RPC over stdio; all diagnostics go to a log file, never
  stdout (which is the protocol pipe).
