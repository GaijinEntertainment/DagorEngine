options gen2
options rtti

module common

require daslib/json_boost public
require daslib/strings_boost public
require daslib/ast public
require daslib/rtti public
require math public
require strings public
require daslib/fio public
require daslib/command_line public

var ast_grep_cmd : string

struct ContentItem {
    @rename = "type" _type : string
    text : string
}

struct ToolResult {
    content : array<ContentItem>
    isError : bool
}

// query matching: case-insensitive substring by default, "=foo" for exact case-sensitive
def symbol_matches(name, query : string) : bool {
    if (!empty(query) && first_character(query) == '=') return name == slice(query, 1)
    return find(to_lower(name), to_lower(query)) >= 0
}

// True when `name` looks like a path-qualified module ("daslib/ast", "foo/bar").
// Used to gate the auto-`daslib/` fallback: only single-token names get rewritten.
def is_qualified_module_name(name : string) : bool {
    return find(name, "/") >= 0
}

// Result of an attempt by an MCP tool helper that compiles a stub-with-`require`.
// `retryable=false` means the failure happened before compile (e.g. temp-file write),
// so retrying with `daslib/<name>` cannot help.
struct TryResult {
    text : string
    ok : bool
    retryable : bool
}

def make_tool_result(text : string; is_error : bool = false) : string {
    var result : ToolResult
    result.isError = is_error
    result.content |> emplace(ContentItem(_type = "text", text = text))
    return sprint_json(result, false)
}

// ── Cross-tree guard ─────────────────────────────────────────────────
// When a tool is asked about a file that lives OUTSIDE this server's root (a different git
// worktree), the answer is produced against THIS root's module sources and THIS binary's
// compiled-in C++ bindings — so it can be stale/wrong for that file (missing symbols, outdated
// signatures). Detect it and prepend a warning so the caller knows the result is suspect.

// Walk up from `start_dir` to the nearest ancestor holding a `.git` entry — a directory in the
// main tree, a file in a linked worktree; `fexist` catches both. "" if none within a bounded walk.
def private git_tree_root(start_dir : string) : string {
    var d = normalize(start_dir)
    for (_ in range(64)) {
        if (fexist(path_join(d, ".git"))) {
            return d
        }
        let up = dir_name(d)
        if (empty(up) || up == d) {
            break
        }
        d = up
    }
    return ""
}

// How to make a cross-tree worktree queryable: depends on whether it's been bootstrapped.
def private bootstrap_hint(tree : string) : string {
    if (empty(tree)) {
        return "couldn't locate its git root"
    }
    let has_bin = fexist(path_join(tree, "bin/daslang")) || fexist(path_join(tree, "bin/daslang.exe"))
    let has_mcp = fexist(path_join(tree, ".mcp.json"))
    if (has_bin && has_mcp) {
        return "that worktree is bootstrapped — start a session rooted in {tree} to query it correctly"
    }
    if (has_bin) {
        return "that worktree has a binary but no MCP config — run `daslang utils/mcp/setup.das -- --root {tree} --no-build`, then query from a session rooted there"
    }
    return "that worktree isn't bootstrapped — run `daslang utils/mcp/setup.das -- --root {tree}`, then query from a session rooted there"
}

// A `relative()` result escapes its base only when its first segment is `..` — exactly ".." or a
// "../" prefix. A segment that merely begins with ".." (a dir named "..foo") stays inside the base.
def private escapes_root(rel : string) : bool {
    return rel == ".." || starts_with(rel, "../")
}

//! The cross-tree warning for a `file` argument — "" when every path it names is relative
//! (server-cwd-relative, so in-tree) or under the server's root (`project_root` if given, else the
//! process cwd). Parses the arg the way the tools do (`parse_file_list`: comma/newline split,
//! whitespace-stripped, empties skipped, globs expanded), so the guard sees the same files, and
//! warns on the FIRST cross-tree path. Public so it can be unit-tested directly.
def crosstree_warning(file_arg, project_root : string) : string {
    if (empty(file_arg)) {
        return ""
    }
    // resolve the effective root to an ABSOLUTE path — a relative project_root would otherwise make
    // relative_result fail for every file and silence the guard entirely
    let raw_root = empty(project_root) ? getcwd() : project_root
    let root = is_absolute(raw_root) ? normalize(raw_root) : normalize(path_join(getcwd(), raw_root))
    var paths : array<string>
    parse_file_list(file_arg, paths)
    for (p in paths) {
        // relative paths resolve against the server's own cwd, so they are in-tree by construction
        if (!is_absolute(p)) {
            continue
        }
        let fileabs = normalize(p)
        let rr = relative_result(fileabs, root)
        // in-tree when the relative path computes AND doesn't escape upward; a compute error means
        // different root names / volumes (Windows cross-drive) — that IS outside the root, so warn.
        if (!(rr is error) && !escapes_root(to_generic_path(rr as value))) {
            continue
        }
        let tree = git_tree_root(dir_name(fileabs))
        return ("CROSS-TREE WARNING — this file is in a different worktree than this MCP server.\n" +
                "  file:   {p}\n" +
                "  server: {root}\n" +
                "This server resolves modules AND its binary's compiled-in C++ bindings from its own root, " +
                "so results for this file may be STALE or wrong (missing symbols, outdated signatures). " +
                "{bootstrap_hint(tree)}.")
    }
    return ""
}

//! Prepend the cross-tree warning (if any) as a leading content block of a tool-result JSON.
//! No-op (returns the input unchanged) when the file is in-tree, so the common path pays nothing
//! beyond one path comparison.
def guard_crosstree(result_json, file_arg, project_root : string) : string {
    let warn = crosstree_warning(file_arg, project_root)
    if (empty(warn)) {
        return result_json
    }
    let item = sprint_json(ContentItem(_type = "text", text = warn), false)
    let marker = "\"content\":["
    let pos = find(result_json, marker)
    if (pos < 0) {
        return result_json
    }
    let cut = pos + length(marker)
    // an empty content array ("content":[]) must not get a trailing comma — [item,] is invalid JSON
    let sep = (cut < length(result_json) && slice(result_json, cut, cut + 1) == "]") ? "" : ","
    return slice(result_json, 0, cut) + item + sep + slice(result_json, cut, length(result_json))
}

// Validate the `project` argument shared across MCP subtools. Empty is fine
// (means "no project, default file access"). Non-empty must point at an
// existing regular `.das_project` file. Returns empty on success, otherwise
// a focused error suitable for `make_tool_result(...)`. Catches the common
// failure mode where callers pass a directory or a non-project file and the
// underlying compile_file() surfaces a misleading "missing prerequisite".
def validate_project_arg(project : string) : string {
    if (empty(project)) return ""
    if (!ends_with(project, ".das_project")) return "invalid 'project' argument: '{project}' — expected a path to a .das_project file"
    let st = stat(project)
    if (!st.is_valid) return "'project' file not found: '{project}'"
    if (st.is_dir) return "'project' is a directory, not a .das_project file: '{project}'"
    if (!st.is_reg) return "'project' is not a regular file: '{project}'"
    return ""
}

def write_deduped(var writer : StringBuilderWriter; messages : array<string>; prefix : string = "  ") {
    var counts : table<string; int>
    var order : array<string>
    for (msg in messages) {
        if (key_exists(counts, msg)) {
            counts[msg]++
        } else {
            counts[msg] = 1
            order |> push(msg)
        }
    }
    for (msg in order) {
        let n = counts[msg]
        if (n > 1) {
            writer |> write("{prefix}{msg} (x{n})\n")
        } else {
            writer |> write("{prefix}{msg}\n")
        }
    }
}

def compile_and_simulate(file : string; project : string = ""; blk : block<(program : smart_ptr<Program>; issues : string) : string>) : string {
    return compile_program(file, false, false, project) <| blk
}

def compile_and_simulate_ctx(file : string; project : string = ""; blk : block<(program : smart_ptr<Program>; var ctx : smart_ptr<Context>; issues : string) : string>) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            cop.auto_inline_functions = false   // introspection describes the SOURCE - splices rewrite bodies/references
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    result = "Compilation failed:\n{issues}"
                    had_error = true
                } else {
                    simulate(program) $(sok; var ctx; serrors) {
                        if (!sok) {
                            result = "Simulation failed:\n{serrors}"
                            had_error = true
                            return
                        }
                        result = invoke(blk, program, ctx, string(issues))
                    }
                }
            }
        }
    }
    return make_tool_result(result, had_error)
}

def compile_program(file : string; _export_all : bool; blk : block<(program : smart_ptr<Program>; issues : string) : string>) : string {
    return compile_program(file, _export_all, false, "") <| blk
}

def compile_program(file : string; _export_all : bool; _no_opt : bool; project : string = ""; blk : block<(program : smart_ptr<Program>; issues : string) : string>) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            cop.auto_inline_functions = false   // introspection describes the SOURCE - splices rewrite bodies/references
            if (_export_all) {
                cop.export_all = true
            }
            if (_no_opt) {
                cop.no_optimizations = true
                cop.no_infer_time_folding = true
            }
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    result = "Compilation failed:\n{issues}"
                    had_error = true
                } else {
                    simulate(program) $(sok; _ctx; serrors) {
                        if (!sok) {
                            result = "Simulation failed:\n{serrors}"
                            had_error = true
                            return
                        }
                        result = invoke(blk, program, string(issues))
                    }
                }
            }
        }
    }
    return make_tool_result(result, had_error)
}

def compile_only(file : string; _export_all : bool; project : string = ""; blk : block<(program : smart_ptr<Program>; issues : string) : string>) : string {
    return compile_only(file, _export_all, false, project, false) <| blk
}

def compile_only(file : string; _export_all : bool; _no_opt : bool; project : string = ""; _lint : bool = false; blk : block<(program : smart_ptr<Program>; issues : string) : string>) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            cop.auto_inline_functions = false   // introspection describes the SOURCE - splices rewrite bodies/references
            if (_export_all) {
                cop.export_all = true
            }
            if (_no_opt) {
                cop.no_optimizations = true
                cop.no_infer_time_folding = true
            }
            if (_lint) {
                cop.lint_check = true
            }
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    result = "Compilation failed:\n{issues}"
                    had_error = true
                } else {
                    result = invoke(blk, program, string(issues))
                }
            }
        }
    }
    return make_tool_result(result, had_error)
}

def arg_needs_documenting(tt) : bool {
    return tt.baseType != Type.fakeContext && tt.baseType != Type.fakeLineInfo
}

def write_func_signature(var w : StringBuilderWriter; func) {
    var has_args = false
    for (i in range(length(func.arguments))) {
        if (!arg_needs_documenting(func.arguments[i]._type)) continue
        if (!has_args) {
            write(w, "(")
            has_args = true
        } else {
            write(w, "; ")
        }
        write(w, "{func.arguments[i].name} : {describe(func.arguments[i]._type)}")
    }
    if (has_args) {
        write(w, ")")
    }
    if (func.result != null && !func.result.isVoid) {
        write(w, " : {describe(func.result)}")
    }
    if (int(func.at.line) > 0) {
        write(w, "  // line {int(func.at.line)}")
    }
}

def write_handled_type(var w : StringBuilderWriter; value : Annotation const) {
    write(w, "  handled {value.name}")
    var ann = unsafe(reinterpret<BasicStructureAnnotation?>(value))
    basic_struct_for_each_parent(*ann) $(parent) {
        write(w, " : {parent.name}")
    }
    if (!empty(value.cppName)) {
        write(w, "  // C++ {value.cppName}")
    }
    write(w, "\n")
    for_each_field(*ann) $(name, cppName, xtype, _offset) {
        if (!empty(cppName) && cppName != name) {
            write(w, "    {name} : {describe(xtype)}  // C++ {cppName}\n")
        } else {
            write(w, "    {name} : {describe(xtype)}\n")
        }
    }
}

def write_function_list(var writer : StringBuilderWriter; program : smart_ptr<Program>) {
    let thisMod = get_this_module(program)
    var user_funcs : array<string>
    var generic_instances : array<string>
    var class_methods : array<string>
    for_each_function(thisMod, "") $(func) {
        if (func.flags.builtIn) return
        var line = ""
        if (func.flags._lambda) return
        if (func.fromGeneric != null) {
            line = build_string() $(var w) {
                write(w, "  def {func.fromGeneric.name}")
                write_func_signature(w, func)
            }
            generic_instances |> push(line)
        } elif (func.flags.generated) {
            return
        } elif (func.flags.isClassMethod) {
            line = build_string() $(var w) {
                write(w, "  def {func.name}")
                write_func_signature(w, func)
            }
            class_methods |> push(line)
        } else {
            line = build_string() $(var w) {
                write(w, "  def {func.name}")
                write_func_signature(w, func)
            }
            user_funcs |> push(line)
        }
    }
    if (!empty(user_funcs)) {
        write(writer, "Functions:\n")
        for (f in user_funcs) {
            write(writer, "{f}\n")
        }
    }
    if (!empty(class_methods)) {
        write(writer, "Class methods:\n")
        for (f in class_methods) {
            write(writer, "{f}\n")
        }
    }
    if (!empty(generic_instances)) {
        write(writer, "Generic instances:\n")
        for (f in generic_instances) {
            write(writer, "{f}\n")
        }
    }
}

// ── Executable path ──────────────────────────────────────────────────

// Returns absolute path to the daslang executable, with backslashes on Windows
def get_daslang_exe() : string {
    return get_das_exe()
}

// ── MCP subtool runner ──────────────────────────────────────────────
//
// Why subprocess: in-process compile_file / RTTI walks leak C++-side
// [function_macro] and [call_macro] registrations into the long-lived MCP
// daslang process. A second call against an edited .das file sees the OLD
// macros. Spawning a fresh daslang.exe per call resets that state entirely.
//
// Subtool contract:
//   - Lives at utils/mcp/subtools/<subtool_name>.das.
//   - [export] def main reads positional argv via get_command_line_arguments().
//   - Prints the FINAL tool-result envelope (the full make_tool_result(...) JSON)
//     to stdout. The wrapper passes it through verbatim — no extra wrapping.
//   - Exits 0 on success AND on tool-level failure (e.g. compile errors are a
//     successful tool run that reports isError=true). Exit non-0 ONLY on
//     catastrophic failure (cannot read args, internal panic).
// Inside an MCP subtool's `[export] def main`, locate the caller-supplied
// args inside daslang's full argv. Conventional invocation:
//   daslang.exe utils/mcp/subtools/<name>.das -- <arg1> <arg2> ...
// Returns the args after `--`. Falls back to args[2..] (skip exe + script
// path) if no `--` is present, so subtools can also be invoked manually
// for debugging without the separator.
def subtool_user_args(args : array<string>) : array<string> {
    var out : array<string>
    var i = 0
    while (i < length(args)) {
        if (args[i] == "--") {
            i++
            while (i < length(args)) {
                out |> push(args[i])
                i++
            }
            return <- out
        }
        i++
    }
    out |> reserve(max(0, length(args) - 2))
    for (j in range(2, length(args))) {
        out |> push(args[j])
    }
    return <- out
}

// daslang.exe CLI flags must come BEFORE the script path. `-project_root <P>`
// triggers `require_dynamic_modules` at daslang startup against <P>/modules/*/.das_module,
// so the subtool's compile_file() picks up daspkg-style native paths
// (e.g. require imgui/X resolving via D:/IMGUI_DEMO/modules/dasImgui/.das_module).
// `--` is critical: without it, daslang treats positional argv as extra
// .das files to load AND auto-runs each. With `--`, daslang stops parsing
// its own options and just exposes the rest via get_command_line_arguments().
def public build_subtool_argv(exe, subtool_path : string;
                              args : array<string>;
                              project_root : string;
                              load_modules : array<string> = array<string>()) : array<string> {
    var argv <- [exe]
    if (!empty(project_root)) {
        argv |> push("-project_root")
        argv |> push(project_root)
    }
    argv |> reserve(length(argv) + 2 * length(load_modules) + 2 + length(args))
    for (lm in load_modules) {
        argv |> push("-load_module")
        argv |> push(lm)
    }
    argv |> push(subtool_path)
    argv |> push("--")
    argv |> push_from(args)
    return <- argv
}

def run_mcp_subtool(subtool_name : string; args : array<string>;
                    project_root : string = "";
                    load_modules : array<string> = array<string>();
                    timeout_sec : float = 120.0) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) return make_tool_result("Cannot determine daslang executable path", true)
    let subtool_path = path_join(get_das_root(), "utils/mcp/subtools/{subtool_name}.das")
    let argv <- build_subtool_argv(exe, subtool_path, args, project_root, load_modules)
    var output : string
    let exit_code = run_and_capture(argv, output, timeout_sec)
    if (exit_code == popen_timed_out) return make_tool_result("MCP subtool '{subtool_name}' timed out after {timeout_sec}s:\n{output}", true)
    if (exit_code != 0) return make_tool_result("MCP subtool '{subtool_name}' failed (exit {exit_code}):\n{output}", true)
    // The subtool emits a single-line ToolResult JSON object on stdout, but run_and_capture merges in any
    // compiler / dyn-module diagnostics printed before it — e.g. `Warning: local 'X' shadows global — using
    // local` from a `-load_module` shadow, or a loaded module's compile-error report (which contains daslang
    // source snippets with `{`). Both print at the child's startup, before the subtool's `main`, and a broken
    // module does NOT abort the run (require_dynamic_modules continues), so the subtool still exits 0.
    // The server splices this return value verbatim into the `result` field, so a stray preamble desyncs the
    // JSON-RPC stream (same class as the 776c10658 GC-leak-dump fix). Anchor on the ToolResult envelope marker
    // (`content` is its first field) rather than the first `{`, which a source-snippet preamble would match.
    let envStart = find(output, "\{\"content")
    if (envStart > 0) {
        output = strip(slice(output, envStart))
    }
    return output
}

// ── Temp file utilities ──────────────────────────────────────────────

// Returns a unique temp .das file path that won't collide with concurrent processes.
def make_temp_das_file() : string {
    return path_join(get_das_root(), "_mcp_stub_{ref_time_ticks()}.das")
}

// ── File path utilities ──────────────────────────────────────────────

def resolve_path(path : string) : string {
    if (empty(path)) return get_das_root()
    // `find(path, ":") == 1` preserves Windows drive-relative paths (`C:foo`):
    // std::filesystem doesn't classify them as absolute (no trailing separator),
    // so they'd otherwise be joined under das_root and produce an invalid path.
    if (is_absolute(path) || find(path, ":") == 1) return path
    // Deliberately NOT normalize()d: lexically_normal rewrites every separator to the
    // platform preferred one, so on Windows it would turn the forward slashes of
    // get_das_root() into backslashes and break callers that compare against it.
    return path_join(get_das_root(), path)
}

def make_relative_path(path, root : string) : string {
    var err : string
    let rel = relative(path, root, err)
    return to_generic_path(empty(rel) ? path : rel)
}
