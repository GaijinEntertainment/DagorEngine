options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

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
    if (length(query) > 0 && first_character(query) == '=') {
        return name == slice(query, 1)
    }
    return find(to_lower(name), to_lower(query)) >= 0
}

def make_tool_result(text : string; is_error : bool = false) : string {
    var result : ToolResult
    result.isError = is_error
    result.content |> emplace(ContentItem(_type = "text", text = text))
    return sprint_json(result, false)
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
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
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
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
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
                    simulate(program) $(sok; ctx; serrors) {
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
    var inscope access <- make_file_access(project)
    var result : string
    var had_error = false
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
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
        if (!arg_needs_documenting(func.arguments[i]._type)) {
            continue
        }
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
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)
    basic_struct_for_each_parent(*ann) $(parent) {
        write(w, " : {parent.name}")
    }
    if (!empty(value.cppName)) {
        write(w, "  // C++ {value.cppName}")
    }
    write(w, "\n")
    for_each_field(*ann) $(name, cppName, xtype, offset) {
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
        if (func.flags.builtIn) {
            return
        }
        var line = ""
        if (func.flags._lambda) {
            return
        }
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

// Run an external command, capture stdout+stderr, return exit_code.
// `args[0]` is the executable; remaining elements are positional arguments.
// timeout_sec > 0 kills the process tree after that many seconds (returns popen_timed_out).
// timeout_sec <= 0 means no timeout.
//
// Argv-based: `popen_argv` (Windows: CreateProcess; Unix: fork+execvp) bypasses
// the shell entirely. No cmd.exe quote-stripping, no /bin/sh `$()`/backtick
// expansion — every argv element reaches the child verbatim. Callers don't
// (and shouldn't) quote arguments themselves.
def run_and_capture(args : array<string>; var output : string&; timeout_sec : float = 0.0) : int {
    var captured : string
    let exit_code = unsafe(popen_argv(args, timeout_sec, $(f) {
        if (f != null) {
            captured := unsafe(fread_to_eof(f))
        }
    }))
    output := captured
    return exit_code
}

// ── Temp file utilities ──────────────────────────────────────────────

// Returns a unique temp .das file path that won't collide with concurrent processes.
def make_temp_das_file() : string {
    return "{get_das_root()}/_mcp_stub_{ref_time_ticks()}.das"
}

// ── File path utilities ──────────────────────────────────────────────

def resolve_path(path : string) : string {
    if (empty(path)) {
        return get_das_root()
    }
    if (length(path) > 0 && first_character(path) != '/' && find(path, ":") < 0) {
        return "{get_das_root()}/{path}"
    }
    return path
}

def make_relative_path(path, root : string) : string {
    var result = path
    if (find(result, root) == 0) {
        result = slice(result, length(root) + 1)
    }
    return replace(result, "\\", "/")
}

// Expand a glob pattern (e.g. "utils/mcp/tools/*.das") into a list of file paths
def expand_glob(pattern : string; var result : array<string>) {
    let last_slash = max(rfind(pattern, "/"), rfind(pattern, "\\"))
    var dir_part = "."
    var file_pattern = pattern
    if (last_slash >= 0) {
        dir_part = slice(pattern, 0, last_slash)
        file_pattern = slice(pattern, last_slash + 1)
    }
    var matches : array<string>
    dir(dir_part) $(fname) {
        if (fname == "." || fname == "..") {
            return
        }
        if (glob_match(file_pattern, fname)) {
            if (dir_part == ".") {
                matches |> push(fname)
            } else {
                matches |> push("{dir_part}/{fname}")
            }
        }
    }
    sort(matches)
    result |> reserve(length(result) + length(matches))
    for (m in matches) {
        result |> push(m)
    }
}

// Parse file argument: comma- or newline-separated list of files, dirs, and
// globs (any part may contain `*` / `?` wildcards, expanded via expand_glob).
def parse_file_list(file : string; var result : array<string>) {
    if (find(file, ",") < 0 && find(file, "\n") < 0) {
        let part = strip(file)
        if (empty(part)) {
            return
        }
        if (find(part, "*") >= 0 || find(part, "?") >= 0) {
            expand_glob(part, result)
        } else {
            result |> push(part)
        }
        return
    }
    for (raw in split_by_chars(file, ",\n")) {
        let part = strip(raw)
        if (empty(part)) {
            continue
        }
        if (find(part, "*") >= 0 || find(part, "?") >= 0) {
            expand_glob(part, result)
        } else {
            result |> push(part)
        }
    }
}
