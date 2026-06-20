options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require ../tools/cpp_common.das public
require daslib/ast_boost

//! Subprocess form of `find_symbol`. Argv: <query> <kind> <file> <project> <with_cpp_source> <cpp_dirs>.
//! `cpp_dirs` is an optional comma-separated list of repo-relative paths that scopes the
//! C++ source-redirect lookup. Empty -> use the cached global index over CPP_SEARCH_DIRS.

let MAX_FIND_RESULTS = 50

def module_display_name(mname : string) : string {
    return mname == "$" ? "builtin" : mname
}

def write_location(var w : StringBuilderWriter; at) {
    if (int(at.line) > 0) {
        write(w, "  // line {int(at.line)}")
    }
}

def write_cpp_name(var w : StringBuilderWriter; cppName : string) {
    if (!empty(cppName)) {
        write(w, "  // C++ {cppName}")
    }
}

def write_cpp_redirect(var w : StringBuilderWriter; cppName : string; with_cpp_source : bool; cpp_dirs : array<string>; cpp_entries : array<CppMatch>) {
    if (!with_cpp_source || empty(cppName)) return
    var match : CppMatch
    // empty cpp_dirs → use the cached global C++ index (CPP_SEARCH_DIRS).
    // non-empty → look up against the pre-scanned `cpp_entries` (built once
    // per run_find_symbol call to avoid N rescans for N matched symbols).
    let found = empty(cpp_dirs) ? cpp_lookup_by_name(cppName, match) : cpp_lookup_by_name_in_entries(cppName, cpp_entries, match)
    if (found) {
        write(w, "\n      → cpp: {match.file}:{match.line}")
    } elif (empty(cpp_dirs)) {
        let why = cpp_index_status()
        if (!empty(why)) {
            write(w, "\n      → cpp: {cppName} (index unavailable: {why})")
        } else {
            write(w, "\n      → cpp: {cppName} (not located)")
        }
    } else {
        let dirs_csv = join(cpp_dirs, ",")
        write(w, "\n      → cpp: {cppName} (not located in {dirs_csv})")
    }
}

def get_func_cpp_name(func : Function?) : string {
    if (func.flags.builtIn) {
        let bfn = func as BuiltInFunction
        if (bfn != null) return string(bfn.cppName)
    }
    return ""
}

def struct_has_matching_field(value; query : string) : bool {
    for (fld in value.fields) {
        if (symbol_matches(string(fld.name), query)) return true
    }
    return false
}

def handled_has_matching_field(value : Annotation const; query : string) : bool {
    var found = false
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)
    for_each_field(*ann) $(name, _cppName, _xtype, _offset) {
        if (symbol_matches(name, query)) {
            found = true
        }
    }
    return found
}

def write_struct_fields_highlighted(var w : StringBuilderWriter; value; query : string; field_search : bool) {
    for (fld in value.fields) {
        if (field_search && symbol_matches(string(fld.name), query)) {
            write(w, "  > {fld.name} : {describe(fld._type)}\n")
        } else {
            write(w, "    {fld.name} : {describe(fld._type)}\n")
        }
    }
}

def write_handled_fields_highlighted(var w : StringBuilderWriter; value : Annotation const; query : string; field_search : bool) {
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)
    for_each_field(*ann) $(name, cppName, xtype, _offset) {
        let prefix = field_search && symbol_matches(name, query) ? "  > " : "    "
        if (!empty(cppName) && cppName != name) {
            write(w, "{prefix}{name} : {describe(xtype)}  // C++ {cppName}\n")
        } else {
            write(w, "{prefix}{name} : {describe(xtype)}\n")
        }
    }
}

def search_module(var results : array<string>; var total_found : int&; mname : string; mod : Module?; query : string; kind : string; with_cpp_source : bool; cpp_dirs : array<string>; cpp_entries : array<CppMatch>) {
    let dname = module_display_name(mname)
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "function")) {
        for_each_function(mod, "") $(func) {
            if (total_found >= MAX_FIND_RESULTS || func.flags.privateFunction || func.flags._lambda || func.flags.generated) return
            let fname = string(func.name)
            if (!symbol_matches(fname, query)) return
            let cppName = get_func_cpp_name(func)
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] def {fname}")
                write_func_signature(w, func)
                write_cpp_name(w, cppName)
                write_cpp_redirect(w, cppName, with_cpp_source, cpp_dirs, cpp_entries)
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "generic")) {
        for_each_generic(mod) $(func) {
            if (total_found >= MAX_FIND_RESULTS || func.flags.privateFunction || func.flags.generated) return
            let fname = string(func.name)
            if (fname |> starts_with("```") || !symbol_matches(fname, query)) return
            let cppName = get_func_cpp_name(func)
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] def {fname}")
                write_func_signature(w, func)
                write_cpp_name(w, cppName)
                write_cpp_redirect(w, cppName, with_cpp_source, cpp_dirs, cpp_entries)
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "struct" || kind == "field")) {
        for_each_structure(mod) $(value) {
            if (total_found >= MAX_FIND_RESULTS || value.flags.isLambda || value.flags._generator || value.flags.generated || value.flags.privateStructure) return
            let name_match = symbol_matches(string(value.name), query)
            let field_match = struct_has_matching_field(value, query)
            if ((kind == "field" && !field_match) || (kind != "field" && !name_match && !field_match)) return
            let entry = build_string() $(var w) {
                if (value.flags.isClass) {
                    write(w, "  [{dname}] class {value.name}")
                } else {
                    write(w, "  [{dname}] struct {value.name}")
                }
                write_location(w, value.at)
                write(w, "\n")
                write_struct_fields_highlighted(w, value, query, field_match && !name_match)
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "handled" || kind == "struct" || kind == "field")) {
        module_for_each_annotation(mod) $(value) {
            if (total_found >= MAX_FIND_RESULTS) return
            let is_basic = value.isBasicStructureAnnotation
            let name_match = symbol_matches(string(value.name), query)
            let field_match = is_basic && handled_has_matching_field(value, query)
            if ((kind == "field" && !field_match) || (kind != "field" && !name_match && !field_match)) return
            let cppName = string(value.cppName)
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] handled {value.name}")
                write_cpp_name(w, cppName)
                write_cpp_redirect(w, cppName, with_cpp_source, cpp_dirs, cpp_entries)
                write(w, "\n")
                if (is_basic) {
                    write_handled_fields_highlighted(w, value, query, field_match && !name_match)
                }
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "enum")) {
        for_each_enumeration(mod) $(value) {
            if (total_found >= MAX_FIND_RESULTS || value.isPrivate || !symbol_matches(string(value.name), query)) return
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] enum {value.name}")
                write_location(w, value.at)
                write(w, "\n")
                for (en in value.list) {
                    write(w, "    {en.name}\n")
                }
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "typedef" || kind == "alias")) {
        for_each_typedef(mod) $(name : string#; var value : TypeDeclPtr) {
            if (total_found >= MAX_FIND_RESULTS) return
            let td = value
            if (td.flags.isPrivateAlias) return
            let sname = string(name)
            if (!symbol_matches(sname, query)) return
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] typedef {sname} = {describe(value)}")
                write_location(w, td.at)
            }
            results |> push(entry)
            total_found++
        }
    }
    if (total_found < MAX_FIND_RESULTS && (empty(kind) || kind == "global")) {
        for_each_global(mod) $(value) {
            if (total_found >= MAX_FIND_RESULTS || value.flags.private_variable || !symbol_matches(string(value.name), query)) return
            let entry = build_string() $(var w) {
                write(w, "  [{dname}] {value.name} : {describe(value._type)}")
                write_location(w, value.at)
            }
            results |> push(entry)
            total_found++
        }
    }
}

def format_find_results(results : array<string>; total_found : int; query : string; kind : string) : string {
    if (empty(results)) return "No symbols found matching '{query}'" + (empty(kind) ? "" : " (kind={kind})")
    let truncated = total_found >= MAX_FIND_RESULTS ? "\n(truncated at {MAX_FIND_RESULTS} results)" : ""
    return build_string() $(var writer) {
        write(writer, "Found {length(results)} symbol(s) matching '{query}':\n")
        for (r in results) {
            write(writer, "{r}\n")
        }
        write(writer, truncated)
    }
}

def run_find_symbol(query : string; kind : string = ""; file : string = ""; project : string = ""; with_cpp_source : bool = false; cpp_dirs : array<string>) : string {
    if (empty(query)) return make_tool_result("'query' must not be empty", true)
    // When cpp_dirs is non-empty, scan ONCE per call and reuse the entries
    // for every per-symbol redirect lookup. The previous code re-scanned
    // inside cpp_lookup_by_name_scoped on every match — up to MAX_FIND_RESULTS
    // (50) rescans of the same dirs per query.
    var cpp_entries : array<CppMatch>
    if (with_cpp_source && !empty(cpp_dirs)) {
        var err : string
        cpp_entries <- cpp_run_scan(cpp_dirs, "", err)
    }
    if (!empty(file)) {
        return compile_and_simulate(file, project) $(program; _issues) {
            var results : array<string>
            var total_found = 0
            program_for_each_module(program) $(mod) {
                let mname = string(mod.name)
                if (mname == "__main__" || empty(mname)) return
                search_module(results, total_found, mname, mod, query, kind, with_cpp_source, cpp_dirs, cpp_entries)
            }
            return format_find_results(results, total_found, query, kind)
        }
    } else {
        var results : array<string>
        var total_found = 0
        program_for_each_registered_module() $(mod) {
            let mname = string(mod.name)
            if (mname == "__main__" || empty(mname)) return
            search_module(results, total_found, mname, mod, query, kind, with_cpp_source, cpp_dirs, cpp_entries)
        }
        return make_tool_result(format_find_results(results, total_found, query, kind))
    }
}

def parse_cpp_dirs(s : string) : array<string> {
    var out : array<string>
    if (empty(s)) return <- out
    for (part in split(s, ",")) {
        let trimmed = strip(part)
        if (!empty(trimmed)) {
            out |> push(trimmed)
        }
    }
    return <- out
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 5) {
        print(make_tool_result("find_symbol subtool: expected 5+ args (query, kind, file, project, with_cpp_source[, cpp_dirs]), got {length(args)}", true))
        return
    }
    let query = args[0]
    let kind = args[1]
    let file = args[2]
    let project = args[3]
    let with_cpp_source = args[4] == "true"
    let cpp_dirs <- length(args) >= 6 ? parse_cpp_dirs(args[5]) : default<array<string>>
    print(run_find_symbol(query, kind, file, project, with_cpp_source, cpp_dirs))
}
