options gen2
options rtti

require protocol_core public
require tools/compile_check public
require tools/list_functions public
require tools/list_types public
require tools/run_test public
require tools/list_module_api public
require tools/list_modules public
require tools/find_symbol public
require tools/format_file public
require tools/run_script public
require tools/ast_dump public
require tools/list_requires public
require tools/convert_to_gen2 public
require tools/goto_definition public
require tools/type_of public
require tools/find_references public
require tools/program_log public
require tools/eval_expression public
require tools/describe_type public
require tools/aot public
require tools/lint_tool public
require tools/detect_duplicates public
require tools/export_corpus public
require tools/judge_duplicates public
require tools/find_dupe public
require tools/live public

// The full daslang compiler-backed tool set: compile/lint/AOT/introspection,
// dupe detection, and the live-reload control surface.
def build_das_tools(var reg : array<ToolDef>) { // nolint:STYLE038 — flat tool-registration table, one emplace per tool
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "compile_check",
            "Compile daScript file(s) and return errors or success. Supports single file, comma-separated list, or glob pattern (e.g. 'utils/mcp/tools/*.das'). Batch mode reports pass/fail per file with summary.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to .das file, comma-separated paths, or glob pattern (e.g. 'dir/*.das')"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (array of CompileResult with file, success, errors, warnings)")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "json"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_compile_check(arg1, project, project_root, arg2 == "true", load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_file_tool("list_functions", "Compile a daScript file and list all functions (after macro expansion)"),
        required <- ["file"],
        arg_names <- ["file"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_list_functions(arg1, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_file_tool("list_types", "Compile a daScript file and list all structs, classes, enums, and type aliases"),
        required <- ["file"],
        arg_names <- ["file"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_list_types(arg1, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "run_test",
            "Run dastest on a .das test file and return pass/fail results",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das test file"),
                "timeout" => PropertySchema(_type = "string", description = "Timeout in seconds (default: 120). Process tree is killed if exceeded"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (TestSummary with file, tests, total, passed, failed, errors, skipped, success, time)"),
                "failures_only" => PropertySchema(_type = "string", description = "If 'true', only show failed tests in output")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "timeout", "json", "failures_only"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_run_test(arg1, project, project_root, arg2, arg3 == "true", arg4 == "true", load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "format_file",
            "Format .das file(s) using the daslang code formatter. Supports single file, comma-separated list, or glob pattern (e.g. 'daslib/*.das'). Returns JSON array with file, status, and message fields; status is 'formatted', 'already_formatted', or 'error'.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to .das file, comma-separated paths, or glob pattern (e.g. 'dir/*.das')")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_format_file(arg1)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "run_script",
            "Run a daslang script and return its stdout output. Provide either a file path or inline code.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file to run"),
                "code" => PropertySchema(_type = "string", description = "Inline daslang code to run (written to temp file and executed)"),
                "timeout" => PropertySchema(_type = "string", description = "Timeout in seconds (default: 30). Process tree is killed if exceeded"),
                "track_allocations" => PropertySchema(_type = "string", description = "If 'true', enable heap allocation tracking and append a heap report at exit (shows where each allocation came from)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            []),
        any_required <- ["file", "code"],
        arg_names <- ["file", "code", "timeout", "track_allocations"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_run_script(arg1, arg2, arg3, arg4 == "true", project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "eval_expression",
            "Evaluate a daslang expression and return its printed result. Wraps the expression in a string builder print statement. Use 'require' to add module imports (comma-separated).",
            {
                "expression" => PropertySchema(_type = "string", description = "daslang expression to evaluate (e.g. 'to_float(42) + 1.0', 'length(\"hello\")')"),
                "require" => PropertySchema(_type = "string", description = "Comma-separated module imports (e.g. 'math', 'strings, daslib/json'). On compile failure, single-token names are retried under 'daslib/'."),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["expression"]),
        required <- ["expression"],
        arg_names <- ["expression", "require"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_eval_expression(arg1, arg2, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "describe_type",
            "Describe a type in detail: fields, methods, enum values, bitfield flags, variant options. Works for structs, classes, handled types, enums, bitfields, variants, tuples, and typedefs.",
            {
                "name" => PropertySchema(_type = "string", description = "Type name (e.g. 'TypeDecl', 'ExprCall', 'Type', 'FunctionFlags')"),
                "module" => PropertySchema(_type = "string", description = "Module to require for the type (e.g. 'ast', 'daslib/json'). Single-token names that fail to resolve are auto-retried as 'daslib/<name>'."),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["name"]),
        required <- ["name"],
        arg_names <- ["name", "module"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_describe_type(arg1, arg2, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "ast_dump",
            "Dump AST of a daslang expression or function. mode='source' shows post-macro daslang code (via describe), mode='ast' (default) shows S-expression with node types.",
            {
                "expression" => PropertySchema(_type = "string", description = "A daslang expression to dump (e.g. 'a.b[0]', 'foo(x, y + 1)')"),
                "file" => PropertySchema(_type = "string", description = "Path to a .das file - dumps AST of compiled functions"),
                "function" => PropertySchema(_type = "string", description = "Function name to dump (optional, defaults to all functions)"),
                "mode" => PropertySchema(_type = "string", description = "Output mode: 'ast' (default) for S-expression, 'source' for post-macro daslang code"),
                "lineinfo" => PropertySchema(_type = "string", description = "If 'true', include LineInfo (file, line:col spans) on each AST node. Only for mode='ast'"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            []),
        any_required <- ["expression", "file"],
        arg_names <- ["expression", "file", "function", "mode", "lineinfo"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_ast_dump(arg1, arg2, arg3, arg4, arg5, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "list_modules",
            "List all available daslang modules (builtin and daslib)",
            {
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (ModuleList with builtin and daslib arrays)")
            },
            []),
        arg_names <- ["json"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_list_modules(arg1 == "true")))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "find_symbol",
            "Search for a symbol (function, struct, enum, global) across all registered modules by substring match",
            {
                "query" => PropertySchema(_type = "string", description = "Case-insensitive substring search. Prefix with '=' for exact case-sensitive match (e.g. '=print')"),
                "kind" => PropertySchema(_type = "string", description = "Limit to kind: 'function', 'generic', 'struct', 'handled', 'field', 'enum', 'global'"),
                "file" => PropertySchema(_type = "string", description = "Optional .das file - if provided, searches all modules loaded by that file (including daslib)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "with_cpp_source" => PropertySchema(_type = "string", description = "If 'true', for each result with a C++ implementation (builtin functions, handled types), append the resolved C++ source location via the cpp index. Adds ~2s on first call (lazy index build)."),
                "cpp_dirs" => PropertySchema(_type = "string", description = "Optional comma-separated repo-relative paths to scope the C++ source-redirect lookup to (e.g. 'src/builtin'). Only consulted when with_cpp_source='true'. Empty -> use the cached global C++ index over CPP_SEARCH_DIRS. Non-empty -> fresh scoped scan, no global cache touched. Useful when you know which subtree the symbol lives in and want to skip the multi-hundred-file index build.")
            },
            ["query"]),
        required <- ["query"],
        arg_names <- ["query", "kind", "file", "with_cpp_source", "cpp_dirs"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_find_symbol(arg1, arg2, arg3, project, project_root, arg4 == "true", arg5, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "list_requires",
            "Compile a .das file and list all require dependencies (direct and transitive)",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (RequiresResult with direct and transitive arrays of RequireEntry)")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "json"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_list_requires(arg1, project, project_root, arg2 == "true", load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "convert_to_gen2",
            "Convert a daslang source file from v1 (indentation-based) syntax to gen2 (braces, parentheses) syntax using the gen1_to_gen2 converter. Returns converted code or writes in place.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file to convert"),
                "inplace" => PropertySchema(_type = "string", description = "If 'true', overwrite the file in place. Otherwise return converted code to stdout.")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "inplace"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_convert_to_gen2(arg1, arg2 == "true")))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "goto_definition",
            "Given a cursor position (file, line, column), resolve the definition of the symbol under the cursor. Returns the definition location, kind, and source snippet.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
                "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
                "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
                "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "with_cpp_source" => PropertySchema(_type = "string", description = "If 'true' and the resolved symbol is a builtin function or a handled type, append the resolved C++ source location. Adds ~2s on first call (lazy index build)."),
                "cpp_dirs" => PropertySchema(_type = "string", description = "Optional comma-separated repo-relative paths to scope the C++ source-redirect lookup to (e.g. 'src/builtin'). Only consulted when with_cpp_source='true'. Empty -> cached global C++ index. Non-empty -> fresh scoped scan, no global cache touched.")
            },
            ["file", "line", "column"]),
        required <- ["file", "line", "column"],
        arg_names <- ["file", "line", "column", "no_opt", "with_cpp_source", "cpp_dirs"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_goto_definition(arg1, arg2, arg3, arg4, project, project_root, arg5 == "true", arg6, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "type_of",
            "Given a cursor position (file, line, column), return the type of the expression under the cursor. Shows all expressions at position from innermost to outermost with their resolved types.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
                "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
                "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
                "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["file", "line", "column"]),
        required <- ["file", "line", "column"],
        arg_names <- ["file", "line", "column", "no_opt"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_type_of(arg1, arg2, arg3, arg4, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "find_references",
            "Find all references to the symbol under the cursor. Returns locations where the symbol is used.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
                "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
                "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
                "scope" => PropertySchema(_type = "string", description = "Search scope: 'file' (default) for current file only, 'all' for all loaded modules"),
                "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["file", "line", "column"]),
        required <- ["file", "line", "column"],
        arg_names <- ["file", "line", "column", "scope", "no_opt"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_find_references(arg1, arg2, arg3, arg4, arg5, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "program_log",
            "Produce post-compilation program text (like 'options log'). Shows all types, globals, and functions after macro expansion, template instantiation, and inference.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file to compile"),
                "function" => PropertySchema(_type = "string", description = "Limit output to a specific function name (optional)"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "function"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_program_log(arg1, arg2, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "list_module_api",
            "List all functions, types, enums, globals, and annotations exported by a daslang module (e.g. 'math', 'strings', 'fio', 'ast', 'daslib/json')",
            {
                "module" => PropertySchema(_type = "string", description = "Module name (e.g. 'math', 'strings', 'fio', 'ast', 'daslib/json'). Single-token names that fail to resolve are auto-retried as 'daslib/<name>'."),
                "filter" => PropertySchema(_type = "string", description = "Substring filter on names (e.g. 'for_each', 'json')"),
                "section" => PropertySchema(_type = "string", description = "Limit to section: 'functions', 'generics', 'operators', 'structs', 'handled', 'enums', 'globals', 'annotations'"),
                "compact" => PropertySchema(_type = "string", description = "Set to 'true' for compact output: function signatures show types only (no arg names), structs/enums omit fields/values. Use with large modules like 'ast', then drill down with describe_type or filter"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["module"]),
        required <- ["module"],
        arg_names <- ["module", "filter", "section", "compact"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_list_module_api(arg1, arg2, arg3, arg4, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "aot",
            "Generate AOT (ahead-of-time) C++ code for a daScript file or a single function. Without 'function', returns the full AOT output. With 'function', returns C++ for that function only. If multiple functions match, returns the list of matches.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
                "function" => PropertySchema(_type = "string", description = "Function name to extract AOT for. Supports exact name, method name (matches ClassName`method), and generic origin."),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "function"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_aot(arg1, arg2, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "lint",
            "Run lint and performance checks on daScript file(s). Combines paranoid lint (unused variables, const suggestions, unreachable code, naming, redundant reinterpret casts) with performance lint (PERF001-012: string concat in loops, character_at misuse, push without reserve, unnecessary string/get_ptr conversions, redundant move-return, string(das_string) in strings functions) and style lint (STYLE001-011: pipe/block syntax, rtti is, if true, decl-then-assign). Supports single file, comma-separated list, or glob pattern.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to .das file, comma-separated paths, or glob pattern"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_lint(arg1, project, project_root, load_modules)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "export_corpus",
            "Build a corpus.json from one or more .das files/dirs/globs. Output is the same shape `detect_duplicates` expects: a JSON dump of every user function's canonical token stream and source location. Run this once over a body of code (e.g. 'daslib', 'tests', or your project) to produce a corpus, then point `detect_duplicates` at it. Shells out to `daslang utils/detect-dupe/main.das` (daslang's `require` grammar can't use hyphenated path components, so the wrapper invokes the CLI as a subprocess). Pass `paths_file` to scope the export to an explicit precomputed list (e.g. files in a PR diff); pass `workers` to parallelize across child processes.",
            {
                "paths" => PropertySchema(_type = "string", description = "Files, directories, or globs to scan. Comma- or newline-delimited. Directories are walked recursively. Skips `_*` directories, hidden directories, `builtin.das`, `daslib/debugger.das`, and `daslib/profiler.das`. Optional when `paths_file` is given; otherwise required."),
                "paths_file" => PropertySchema(_type = "string", description = "Path to a newline-delimited file of paths. Skips blank lines and lines starting with `#`. Composes with `paths` (union; deduped). Use this for PR-scoped runs where you'd otherwise hit ARG_MAX (e.g. piping `git diff --name-only` to a file)."),
                "out" => PropertySchema(_type = "string", description = "Output corpus.json path. Overwrites if it exists. Refuses to write a partial corpus when any input file fails to compile (matches the CLI's --export-functions behavior)."),
                "min_tokens" => PropertySchema(_type = "string", description = "Minimum canonical-token count per record (default 0 — keep every user function). Records below the threshold are dropped at export time. Most callers should leave this at 0 and let `detect_duplicates --min-tokens` filter at query time."),
                "workers" => PropertySchema(_type = "string", description = "Worker count for parallel compilation. 0 = auto (= hardware threads). 1 = sequential. Files are split into N contiguous chunks; each chunk is compiled by a child detect-dupe process; shards are merged in chunk-index order so the output is byte-identical across worker counts. Below 16 input files the export stays sequential regardless.")
            },
            ["out"]),
        required <- ["out"],
        any_required <- ["paths", "paths_file"],
        arg_names <- ["paths", "paths_file", "out", "min_tokens", "workers"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_export_corpus(arg1, arg2, arg3, arg4, arg5)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "detect_duplicates",
            "Find functions in `paths` that duplicate or closely resemble functions in a pre-built corpus (an `--export-functions` JSON from `detect-dupe`). Returns a per-candidate JSON report: each input function, its top exact-cluster siblings, and its top fuzzy partners with similarity scores. Use to answer 'did I just write something that already exists?' during PR authoring or review.",
            {
                "paths" => PropertySchema(_type = "string", description = "One or more .das files to compare against the corpus. Comma- or newline-delimited list, or a glob (e.g. 'daslib/perf_lint.das' or 'src/foo/*.das'). Directories are not expanded — use a glob. Newline support lets `git diff --name-only` output be piped in directly."),
                "corpus" => PropertySchema(_type = "string", description = "Path to a corpus.json produced by `detect-dupe --export-functions`. Records whose `file` matches a `paths` entry are dropped from the corpus and replaced by the freshly-compiled version (so the candidate file is compared against the rest of the world, not against itself)."),
                "threshold" => PropertySchema(_type = "string", description = "Fuzzy similarity floor 0..1 (default 0.7). Score is sqrt(jaccard × len_ratio)."),
                "min_tokens" => PropertySchema(_type = "string", description = "Minimum canonical-token count per function (default 8). Filters trivial wrappers."),
                "top" => PropertySchema(_type = "string", description = "Top-N similar matches per candidate (default 5)."),
                "keep" => PropertySchema(_type = "string", description = "Comma-separated pattern names to KEEP despite the default-skip filter. Special value 'all' disables pattern filtering entirely. Default is empty — all known patterns are skipped (currently: 'visitor', 'dispatch', 'emit'). The skipped count per pattern is reported in the envelope as `patterns_skipped`.")
            },
            ["paths", "corpus"]),
        required <- ["paths", "corpus"],
        arg_names <- ["paths", "corpus", "threshold", "min_tokens", "top", "keep"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_detect_duplicates(arg1, arg2, arg3, arg4, arg5, arg6)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "judge_duplicates",
            "AI judge for detect-dupe clusters. Takes a detect-dupe JSON report and asks Claude (Haiku 4.5 by default) to partition each cluster into real-duplicate groups vs false positives, with a one-line reason per cluster. Returns an envelope with the embedded verdict JSON. Shells out to `daslang utils/find-dupe/main.das` — requires `daspkg install --root utils/find-dupe` first (fetches the `anthropic/anthropic` package) and the `ANTHROPIC_API_KEY` env var. WARNING: sends source code to Anthropic's API — only run on code your data-handling policy permits sharing.",
            {
                "input" => PropertySchema(_type = "string", description = "Path to a detect-dupe JSON report. Produce one with `detect-dupe --json <path>` from the CLI, or use the `find_dupe` MCP tool which writes a `dupes.json` and judges it in one call. (The `detect_duplicates` MCP tool embeds the report inline and deletes its temp file, so its output isn't a reusable path.)"),
                "out" => PropertySchema(_type = "string", description = "Output directory for find_dupe_verdicts.json + .md. Defaults to a fresh temp dir under the project root."),
                "model" => PropertySchema(_type = "string", description = "`haiku` (default — Claude Haiku 4.5) or `sonnet` (Claude Sonnet 4.6)."),
                "max_clusters" => PropertySchema(_type = "string", description = "Hard cap on clusters analyzed after the worth-it gate (default 0 = no cap). Useful for cost control on large reports."),
                "positives_only" => PropertySchema(_type = "string", description = "If `true`, the verdict report keeps only `real`+`partial` rows. Summary still reflects all clusters."),
                "dry_run" => PropertySchema(_type = "string", description = "If `true`, estimate cluster count and token cost without making any API calls. Recommended as a first step.")
            },
            ["input"]),
        required <- ["input"],
        arg_names <- ["input", "out", "model", "max_clusters", "positives_only", "dry_run"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_judge_duplicates(arg1, arg2, arg3, arg4, arg5, arg6)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "find_dupe",
            "Convenience wrapper: runs `detect-dupe` against `paths` to produce a duplicate-cluster JSON, then hands it off to the AI judge in one call. Same daspkg + API-key requirement as `judge_duplicates`. Use this when starting fresh on a directory or PR; use `detect_duplicates` + `judge_duplicates` separately when you already have a curated corpus.",
            {
                "paths" => PropertySchema(_type = "string", description = "Files / directories / globs to scan (comma- or newline-delimited, same syntax as `detect_duplicates`)."),
                "out" => PropertySchema(_type = "string", description = "Output directory. Stores the intermediate `dupes.json` and the verdict report. Defaults to a fresh temp dir under the project root."),
                "model" => PropertySchema(_type = "string", description = "`haiku` (default) or `sonnet`."),
                "threshold" => PropertySchema(_type = "string", description = "Fuzzy similarity floor 0..1 (default 0.7) — passed through to detect-dupe."),
                "max_clusters" => PropertySchema(_type = "string", description = "Hard cap on clusters judged (default 0 = no cap)."),
                "dry_run" => PropertySchema(_type = "string", description = "If `true`, run detect-dupe normally but skip the API calls — returns a token+cost estimate instead of verdicts.")
            },
            ["paths"]),
        required <- ["paths"],
        arg_names <- ["paths", "out", "model", "threshold", "max_clusters", "dry_run"],
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_find_dupe(arg1, arg2, arg3, arg4, arg5, arg6)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_status",
            "Get status of a running daslang-live instance (fps, uptime, paused, dt, has_error). Returns 503 JSON with compilation error if the script failed to compile.",
            { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
            []),
        arg_names <- ["port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_status(arg1)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_error",
            "Get the last compilation error from a running daslang-live instance. Returns null if no error.",
            { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
            []),
        arg_names <- ["port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_error(arg1)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_reload",
            "Trigger a reload of the running daslang-live script. Works even when there is a compilation error.",
            {
                "full" => PropertySchema(_type = "string", description = "If 'true', force full recompile instead of incremental reload"),
                "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
            },
            []),
        arg_names <- ["full", "port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_reload(arg1, arg2)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_pause",
            "Pause or unpause the running daslang-live script. Returns 503 if there is a compilation error.",
            {
                "paused" => PropertySchema(_type = "string", description = "'true' to pause, 'false' to unpause"),
                "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
            },
            ["paused"]),
        required <- ["paused"],
        arg_names <- ["paused", "port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_pause(arg1, arg2)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_command",
            "Dispatch a [live_command] to the running daslang-live instance. Use name='help' to list all available commands. Returns 503 if there is a compilation error.",
            {
                "name" => PropertySchema(_type = "string", description = "Command name (e.g. 'help', 'screenshot', 'cmd_spawn_ball')"),
                "args" => PropertySchema(_type = "string", description = "Command arguments as JSON object string (e.g. '\{\"x\":1\}')"),
                "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
            },
            ["name"]),
        required <- ["name"],
        arg_names <- ["name", "args", "port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_command(arg1, arg2, arg3)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_commands",
            "Dispatch a batch of [live_command]s in one round-trip. Continue-on-error semantics — each command runs independently, response is a JSON array preserving input order with malformed entries surfacing \{\"error\":...\} in their slot. Useful for state-update bursts (e.g. dasimgui).",
            {
                "commands" => PropertySchema(_type = "string", description = "JSON array of \{name,args\} objects, e.g. '[\{\"name\":\"a\",\"args\":\{...\}\},\{\"name\":\"b\"\}]'"),
                "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
            },
            ["commands"]),
        required <- ["commands"],
        arg_names <- ["commands", "port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_commands(arg1, arg2)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_shutdown",
            "Gracefully shut down the running daslang-live instance.",
            { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
            []),
        arg_names <- ["port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_shutdown(arg1)))
    reg |> emplace(ToolDef(
        tool <- make_tool(
            "live_launch",
            "Launch a daslang-live instance with a script file. If an instance is already running, returns its status instead of starting a new one. Polls for up to 10 seconds to confirm startup.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to the .das script file to run"),
                "project" => PROJECT_PROP,
                "project_root" => PROJECT_ROOT_PROP,
                "load_modules" => LOAD_MODULES_PROP,
                "port" => PropertySchema(_type = "string", description = "Live API port to check (default: 9090)")
            },
            ["file"]),
        required <- ["file"],
        arg_names <- ["file", "port"],
        main_thread = true,
        handler = @@(arg1, arg2, arg3, arg4, arg5, arg6, project, project_root : string; load_modules : array<string>) => do_live_launch(arg1, project, project_root, arg2, load_modules)))
}
