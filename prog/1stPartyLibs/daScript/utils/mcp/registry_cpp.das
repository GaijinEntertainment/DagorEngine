options gen2
options rtti

module registry_cpp

require protocol_core public
require tools/grep_usage public
require tools/outline public
require tools/cpp_grep_usage public
require tools/cpp_find_symbol public
require tools/cpp_outline public
require tools/cpp_goto_definition public
require tools/cpp_compile_check public
require tools/cpp_format_file public
require tools/cpp_status public

// The cpp/agnostic subset shared by main.das and cpp_main.das: ast-grep-backed
// .das + .cpp search/outline plus the compile-DB-driven C++ build tools. None
// require the daslang compiler at runtime.
def build_cpp_tools(var reg : array<ToolDef>) {
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "grep_usage",
            "Parse-aware symbol search across .das files using ast-grep + tree-sitter. Finds identifier occurrences excluding comments and strings. Much faster than compilation-based search.",
            {
                "symbol" => PropertySchema(_type = "string", description = "Identifier name to search for (e.g. 'compile_program', 'for_each_function')"),
                "directory" => PropertySchema(_type = "string", description = "Directory to search in (default: project root). Relative to project root."),
                "glob" => PropertySchema(_type = "string", description = "File filter pattern (e.g. 'daslib/*.das', 'utils/**/*.das'). Prefix with ! to exclude (e.g. '!modules/**')"),
                "context_lines" => PropertySchema(_type = "string", description = "Reserved for future context-line control (currently unused; ast-grep returns the matched line)")
            },
            ["symbol"]),
        required <- ["symbol"],
        arg_names <- ["symbol", "directory", "context_lines", "glob"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_grep_usage(arg1, arg2, arg3, arg4)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "outline",
            "List all declarations (functions, structs, classes, enums, globals, typedefs) in a file or set of files using tree-sitter. Works on broken/incomplete code — no compilation needed.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to a .das file (relative to project root)"),
                "glob" => PropertySchema(_type = "string", description = "Glob pattern to scan multiple files (e.g. 'daslib/*.das')")
            },
            []),
        any_required <- ["file", "glob"],
        arg_names <- ["file", "glob"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_outline(arg1, arg2)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_grep_usage",
            "Parse-aware C++ identifier search across .cpp/.h/.hpp/.cc files using ast-grep + tree-sitter-cpp. Skips comments and strings. Searches src/, include/, modules/ by default; build*/, _deps/, 3rdparty/ are always excluded.",
            {
                "symbol" => PropertySchema(_type = "string", description = "Identifier to search for (e.g. 'addExternFunc', 'Module::addFunction')"),
                "directory" => PropertySchema(_type = "string", description = "Directory to search (default: src/ include/ modules/). Repo-relative or absolute."),
                "glob" => PropertySchema(_type = "string", description = "Extra file filter (gitignore syntax, '!pattern' for exclusion). Combined with default excludes."),
                "context_lines" => PropertySchema(_type = "string", description = "Reserved for future context-line control (currently unused; ast-grep returns the matched line)")
            },
            ["symbol"]),
        required <- ["symbol"],
        arg_names <- ["symbol", "directory", "context_lines", "glob"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_grep_usage(arg1, arg2, arg3, arg4)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_find_symbol",
            "Search C++ symbol DECLARATIONS by name + kind across .cpp/.h/.hpp/.cc files. Best-effort name extraction; complex templates and function-pointer typedefs may report partial names. Macro-expanded declarations (e.g. DAS_BIND_FN(foo)) are invisible to ast-grep.",
            {
                "query" => PropertySchema(_type = "string", description = "Case-insensitive substring search. Prefix '=' for exact case-sensitive match (e.g. '=Module')"),
                "kind" => PropertySchema(_type = "string", description = "Limit to kind: 'function', 'class', 'struct', 'enum', 'union', 'typedef', 'namespace', 'macro'. Empty matches any."),
                "directory" => PropertySchema(_type = "string", description = "Directory to search (default: src/ include/ modules/)."),
                "glob" => PropertySchema(_type = "string", description = "Extra file filter (gitignore syntax)")
            },
            ["query"]),
        required <- ["query"],
        arg_names <- ["query", "kind", "directory", "glob"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_find_symbol(arg1, arg2, arg3, arg4)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_outline",
            "List C++ declarations (function, class, struct, enum, union, typedef, namespace, macro) in a file or glob via tree-sitter-cpp. Output groups entries by file and shows containment (methods nested under their class, classes under their namespace). Functions show the full signature; template specializations include their args. Works on broken code; no compile DB needed.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to a single C++ file (.cpp/.h/.hpp/.cc), repo-relative or absolute"),
                "glob" => PropertySchema(_type = "string", description = "Glob pattern to scan multiple files (e.g. 'src/builtin/*.cpp'). Searches src/ include/ modules/ when only `glob` is given."),
                "mode" => PropertySchema(_type = "string", description = "'' (default, auto: tree if any nesting exists, else flat), 'tree' (force indented hierarchy), or 'flat' (one entry per line with `Outer::Inner::name` qualified prefixes)")
            },
            []),
        any_required <- ["file", "glob"],
        arg_names <- ["file", "glob", "mode"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_outline(arg1, arg2, arg3)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_goto_definition",
            "Given a cursor position in a C++ file, return up to 5 plausible definition locations. APPROXIMATE — no scope resolution, no overload disambiguation, no template specialization. For substring/usage search, prefer cpp_grep_usage.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to a C++ file (.cpp/.h/.hpp/.cc)"),
                "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
                "column" => PropertySchema(_type = "string", description = "Column number (1-based)")
            },
            ["file", "line", "column"]),
        required <- ["file", "line", "column"],
        arg_names <- ["file", "line", "column"],
        needs_ast_grep = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_goto_definition(arg1, arg2, arg3)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_compile_check",
            "Syntax-check a C++ translation unit using the project's compile_commands.json (clangd-free). Takes the TU's exact flags from the compile DB, strips the codegen tail, and runs cl /Zs (MSVC) or -fsyntax-only (clang/gcc). Requires a CMake build configured with CMAKE_EXPORT_COMPILE_COMMANDS=ON (Ninja/Makefiles emit it; the Visual Studio generator does NOT — use a side Ninja build dir on Windows). On MSVC the spawned compiler needs a developer environment (INCLUDE/LIB from vcvars), since the compile DB omits system include paths. Headers aren't translation units — pass a .cpp/.cc that includes them.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the C++ translation unit (.cpp/.cc/.cxx), repo-relative or absolute"),
                "build_dir" => PropertySchema(_type = "string", description = "Build directory, or a direct path to compile_commands.json. Empty -> probe build/, build-ninja/, then build*/ under the repo root."),
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (CppCompileResult with file, success, errors, warnings)")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "build_dir", "json"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_compile_check(arg1, arg2, arg3 == "true")))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_build_info",
            "Return the compiler, build directory, full compile command, and derived syntax-only command for a C++ translation unit, read from the project's compile_commands.json. Answers 'what command line compiles this file'. Same compile-DB requirement as cpp_compile_check.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the C++ translation unit (.cpp/.cc/.cxx), repo-relative or absolute"),
                "build_dir" => PropertySchema(_type = "string", description = "Build directory, or a direct path to compile_commands.json. Empty -> probe build/, build-ninja/, then build*/ under the repo root.")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "build_dir"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_build_info(arg1, arg2)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_format_file",
            "Format a C++ file in place with clang-format, but ONLY when a .clang-format is discoverable by walking up from the file. The daScript tree ships none, so this is a no-op-with-message there; it's for external C++ consumers that carry a .clang-format. Requires clang-format on PATH.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the C++ file to format, repo-relative or absolute")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_format_file(arg1)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "cpp_status",
            "Report which external dependencies the cpp/agnostic tools need (ast-grep, a compile_commands.json, clang-format, and the MSVC vcvars INCLUDE env) and whether each is present — so a freshly-installed bundle can self-diagnose why a tool is missing or failing. No compilation, no side effects.",
            {
                "build_dir" => PropertySchema(_type = "string", description = "Build directory (or a path to compile_commands.json) to check. Empty -> probe the server root's build/, build-ninja/, build*/. Pass your project's build dir when running cpp-mcp as a standalone bundle against an external project, or the compile DB check reports the server root, not your project."),
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON instead of the text report")
            },
            []),
        arg_names <- ["build_dir", "json"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_cpp_status(arg1, arg2 == "true")))
}
