options gen2
options rtti

require common public
require daslib/json_boost

//! the C++ modules the process registered, the daslib files under the das root, and the require
//! paths descriptors registered (`module/path`, one per `register_native_path` row)
struct ModuleList {
    builtin : array<string>
    daslib : array<string>
    registered : array<string>
}

//! a module with a file name was compiled from das - the server's own program, a descriptor's
//! helper - and is not a C++ module the process offers
def list_modules_here(json : bool = false) : string {
    var builtin_mods : array<string>
    program_for_each_registered_module() $(mod) {
        let mname = string(mod.name)
        if (mname == "$" || mname == "__main__" || empty(mname) || !empty(mod.fileName)) return
        builtin_mods |> push(mname)
    }
    sort(builtin_mods)
    var daslib_mods : array<string>
    dir(path_join(get_das_root(), "daslib")) $(fname) {
        if (!ends_with(fname, ".das")) return
        daslib_mods |> push(slice(fname, 0, length(fname) - 4))
    }
    sort(daslib_mods)
    var registered : array<string>
    for_each_registered_native_path() $(mod_name, src_path, _dst_path) {
        registered |> push("{mod_name}/{src_path}")
    }
    sort(registered)
    if (json) {
        let result = ModuleList(builtin <- builtin_mods, daslib <- daslib_mods, registered <- registered)
        return make_tool_result(sprint_json(result, false))
    }
    return make_tool_result(build_string() $(var writer) {
        write(writer, "Builtin/C++ modules ({length(builtin_mods)}):\n")
        for (m in builtin_mods) {
            write(writer, "  {m}\n")
        }
        write(writer, "daslib modules ({length(daslib_mods)}):\n")
        for (m in daslib_mods) {
            write(writer, "  daslib/{m}\n")
        }
        write(writer, "Registered require paths ({length(registered)}):\n")
        for (m in registered) {
            write(writer, "  {m}\n")
        }
    })
}

//! with a project root or extra modules the listing comes from a child that scanned them, eager,
//! since a deferred C++ module is not a registered one until something requires it
def do_list_modules(json : bool = false; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    if (empty(project_root) && empty(load_modules)) return list_modules_here(json)
    return run_mcp_subtool("list_modules", [json ? "true" : "false"], project_root, load_modules, eager = true)
}
