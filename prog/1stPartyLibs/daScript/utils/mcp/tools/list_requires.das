options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/json_boost

struct RequireEntry {
    name : string
    is_public : bool
    source : string  // "builtin" or file path
}

struct RequiresResult {
    direct : array<RequireEntry>
    transitive : array<RequireEntry>
}

def module_kind(mod : Module?) : string {
    if (mod == null) {
        return "missing"
    }
    if (!empty(mod.fileName)) {
        return string(mod.fileName)
    }
    return "builtin"
}

def do_list_requires(file : string; project : string = ""; json : bool = false) : string {
    return compile_program(file, true, false, project) <| $(program; issues) {
        // direct requires from allRequireDecl
        var direct_text : array<string>
        var direct_entries : array<RequireEntry>
        var direct_modules : array<string>
        for_each_require_declaration(program) <| $(var mod : Module?; mod_name : string#; mod_file : string#; var is_public : bool; at : LineInfo&) {
            let kind = module_kind(mod)
            if (!json) {
                let pub_marker = is_public ? " public" : ""
                direct_text |> push("  require {mod_name}{pub_marker}  // {kind}")
            } else {
                direct_entries |> emplace(RequireEntry(name = string(mod_name), is_public = is_public, source = kind))
            }
            direct_modules |> push(string(mod_name))
        }
        // transitive: walk dependencies of the main module, skip "$" (always implicit)
        var transitive_text : array<string>
        var transitive_entries : array<RequireEntry>
        let thisMod = get_this_module(program)
        var visited : table<string; bool>
        for (dm in direct_modules) {
            visited |> insert(dm, true)
        }
        visited |> insert("$", true)
        var queue : array<Module?>
        module_for_each_dependency(thisMod) <| $(var dep : Module?; var is_pub : bool) {
            let dep_name = string(dep.name)
            if (!key_exists(visited, dep_name)) {
                visited |> insert(dep_name, true)
                queue |> push(dep)
            }
        }
        var idx = 0
        while (idx < length(queue)) {
            let mod = queue[idx]
            idx++
            let mod_name = string(mod.name)
            let kind = module_kind(mod)
            if (!json) {
                transitive_text |> push("  {mod_name}  // {kind}")
            } else {
                transitive_entries |> emplace(RequireEntry(name = mod_name, source = kind))
            }
            module_for_each_dependency(mod) <| $(var dep : Module?; var is_pub : bool) {
                let dep_name = string(dep.name)
                if (!key_exists(visited, dep_name)) {
                    visited |> insert(dep_name, true)
                    queue |> push(dep)
                }
            }
        }
        if (json) {
            sort(transitive_entries) <| $(a, b : RequireEntry) {
                return a.name < b.name
            }
            var result = RequiresResult(direct <- direct_entries, transitive <- transitive_entries)
            return sprint_json(result, false)
        }
        sort(transitive_text)
        return build_string() <| $(var w) {
            if (!empty(direct_text)) {
                write(w, "Direct requires:\n")
                for (d in direct_text) {
                    write(w, "{d}\n")
                }
            }
            if (!empty(transitive_text)) {
                if (!empty(direct_text)) {
                    write(w, "\n")
                }
                write(w, "Transitive dependencies ({length(transitive_text)}):\n")
                for (t in transitive_text) {
                    write(w, "{t}\n")
                }
            }
            if (empty(direct_text) && empty(transitive_text)) {
                write(w, "No requires found.\n")
            }
        }
    }
}
