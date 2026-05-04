options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/json_boost

struct ModuleList {
    builtin : array<string>
    daslib : array<string>
}

def do_list_modules(json : bool = false) : string {
    var builtin_mods : array<string>
    program_for_each_registered_module() <| $(mod) {
        let mname = string(mod.name)
        if (mname == "$" || mname == "__main__" || empty(mname)) {
            return
        }
        builtin_mods |> push(mname)
    }
    sort(builtin_mods)
    var daslib_mods : array<string>
    dir("daslib") <| $(fname) {
        if (!ends_with(fname, ".das")) {
            return
        }
        daslib_mods |> push(slice(fname, 0, length(fname) - 4))
    }
    sort(daslib_mods)
    if (json) {
        var result = ModuleList(builtin <- builtin_mods, daslib <- daslib_mods)
        return make_tool_result(sprint_json(result, false))
    }
    return make_tool_result(build_string() <| $(var writer) {
        write(writer, "Builtin/C++ modules ({length(builtin_mods)}):\n")
        for (m in builtin_mods) {
            write(writer, "  {m}\n")
        }
        write(writer, "daslib modules ({length(daslib_mods)}):\n")
        for (m in daslib_mods) {
            write(writer, "  daslib/{m}\n")
        }
    })
}
