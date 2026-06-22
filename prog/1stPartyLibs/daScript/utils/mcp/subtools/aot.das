options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/aot_cpp
require daslib/ast_boost
require strings

//! Subprocess form of `aot`. Argv: <file> <func_name> <project>.

struct AotFuncInfo {
    name : string
    aot_name : string
    signature : string
}

def collect_aot_functions(program : smart_ptr<Program>) : array<AotFuncInfo> {
    var result : array<AotFuncInfo>
    let this_mod = program.getThisModule
    this_mod |> for_each_module_function($(fn) {
        if (fn.flags.noAot || fn.moreFlags.isTemplate || !fn.flags.used || fn.flags.builtIn || fn.flags.generated) return
        let sig = build_string() $(var w) {
            write(w, fn.name)
            write(w, "(")
            var first = true
            for (i in range(length(fn.arguments))) {
                if (fn.arguments[i]._type.baseType == Type.fakeContext || fn.arguments[i]._type.baseType == Type.fakeLineInfo) continue
                if (!first) {
                    write(w, "; ")
                }
                first = false
                write(w, "{fn.arguments[i].name} : {describe(fn.arguments[i]._type)}")
            }
            write(w, ")")
            if (fn.result != null && !fn.result.isVoid) {
                write(w, " : {describe(fn.result)}")
            }
        }
        result |> emplace(AotFuncInfo(
            name = string(fn.name),
            aot_name = aotFuncName(fn),
            signature = sig
        ))
    })
    return <- result
}

def match_function_name(fn_name, query : string) : bool {
    if (fn_name == query) return true
    let bt = find(fn_name, "`")
    return bt >= 0 && slice(fn_name, bt + 1) == query
}

def match_function_generic(fn_name, query : string; program : smart_ptr<Program>) : bool {
    var found = false
    let this_mod = program.getThisModule
    this_mod |> for_each_module_function($(fn) {
        if (found || fn.name != fn_name) return
        if (fn.fromGeneric != null && match_function_name(string(fn.fromGeneric.name), query)) {
            found = true
        }
    })
    return found
}

def run_aot(file, func_name : string; project : string = "") : string {
    if (empty(file)) return make_tool_result("missing 'file' argument", true)
    if (empty(func_name)) return compile_and_simulate_ctx(file, project) $(program : smart_ptr<Program>; var ctx : smart_ptr<Context>; issues : string) {
            var warnings = ""
            if (!empty(issues)) {
                warnings = "Warnings:\n{issues}\n"
            }
            var result : string
            using() $(var cop : CodeOfPolicies) {
                cop.aot = false
                cop.aot_module = true
                result = "{warnings}{_::run_aot(program.get_ptr(), ctx.get_ptr(), cop)}"
            }
            return result
        }
    return compile_and_simulate_ctx(file, project) $(program : smart_ptr<Program>; var ctx : smart_ptr<Context>; _issues : string) {
        let all_funcs <- collect_aot_functions(program)
        var matches <- [for (i in range(length(all_funcs))); i; where all_funcs[i].name == func_name]
        if (empty(matches)) {
            for (i in range(length(all_funcs))) {
                if (all_funcs[i].aot_name == func_name) {
                    matches |> push(i)
                }
            }
        }
        if (empty(matches)) {
            for (i in range(length(all_funcs))) {
                if (match_function_name(all_funcs[i].name, func_name) || match_function_generic(all_funcs[i].name, func_name, program)) {
                    matches |> push(i)
                }
            }
        }
        if (empty(matches)) {
            let sigs <- [for (fi in all_funcs); fi.signature]
            return "No function matching '{func_name}' found.\nAvailable functions:\n" + join(sigs, "\n")
        }
        if (length(matches) == 1) {
            let matched_name = all_funcs[matches[0]].aot_name
            var result : string
            using() $(var cop : CodeOfPolicies) {
                cop.aot = false
                cop.aot_module = true
                result = run_aot_function(program.get_ptr(), ctx.get_ptr(), cop, matched_name)
            }
            if (empty(result)) return "Function '{matched_name}' produced no AOT output (marked noAot or no_aot)."
            return result
        }
        let lines <- [for (idx in matches); "{all_funcs[idx].signature}  [{all_funcs[idx].aot_name}]"]
        return "Multiple functions match '{func_name}':\n" + join(lines, "\n") + "\nUse the bracketed name for exact selection."
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 3) {
        print(make_tool_result("aot subtool: expected 3 args (file, func_name, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let func_name = args[1]
    let project = args[2]
    // run_aot -> run_aot_function builds AST during C++ codegen; without this
    // guard those nodes leak and the GC dumps "GC APP LEAK ..." to stdout on
    // exit, corrupting the JSON-RPC frame the MCP server sends to the client.
    ast_gc_guard() {
        print(run_aot(file, func_name, project))
    }
}
