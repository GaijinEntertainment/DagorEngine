options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/aot_cpp
require daslib/ast_boost
require strings

struct AotFuncInfo {
    name : string       // fn.name (daslang name)
    aot_name : string   // mangled C++ name with hash
    signature : string  // human-readable signature
}

def collect_aot_functions(program : smart_ptr<Program>) : array<AotFuncInfo> {
    var result : array<AotFuncInfo>
    let this_mod = program.getThisModule
    this_mod |> for_each_module_function($(fn) {
        if (fn.flags.noAot || fn.moreFlags.isTemplate || !fn.flags.used) {
            return
        }
        if (fn.flags.builtIn || fn.flags.generated) {
            return
        }
        var sig = build_string() <| $(var w) {
            write(w, string(fn.name))
            write(w, "(")
            var first = true
            for (i in range(length(fn.arguments))) {
                if (fn.arguments[i]._type.baseType == Type.fakeContext || fn.arguments[i]._type.baseType == Type.fakeLineInfo) {
                    continue
                }
                if (!first) {
                    write(w, "; ")
                }
                first = false
                write(w, "{string(fn.arguments[i].name)} : {describe(fn.arguments[i]._type)}")
            }
            write(w, ")")
            if (fn.result != null && !fn.result.isVoid) {
                write(w, " : {describe(fn.result)}")
            }
        }
        result |> emplace(AotFuncInfo(
            name = string(fn.name),
            aot_name = aotFuncName(fn.get_ptr()),
            signature = sig
        ))
    })
    return <- result
}

def match_function_name(fn_name, query : string) : bool {
    if (fn_name == query) {
        return true
    }
    // method match: query "foo" matches "ClassName`foo"
    let bt = find(fn_name, "`")
    if (bt >= 0 && slice(fn_name, bt + 1) == query) {
        return true
    }
    return false
}

def match_function_generic(fn_name, query : string; program : smart_ptr<Program>) : bool {
    var found = false
    let this_mod = program.getThisModule
    this_mod |> for_each_module_function($(fn) {
        if (found) {
            return
        }
        if (string(fn.name) != fn_name) {
            return
        }
        if (fn.fromGeneric != null && match_function_name(string(fn.fromGeneric.name), query)) {
            found = true
        }
    })
    return found
}

def do_aot(file, func_name : string; project : string = "") : string {
    if (empty(file)) {
        return make_tool_result("missing 'file' argument", true)
    }

    // full file AOT when no function specified
    if (empty(func_name)) {
        return compile_and_simulate_ctx(file, project) <| $(program : smart_ptr<Program>; var ctx : smart_ptr<Context>; issues : string) {
            var warnings = ""
            if (!empty(issues)) {
                warnings = "Warnings:\n{issues}\n"
            }
            var result : string
            using() <| $(var cop : CodeOfPolicies) {
                cop.aot = false
                cop.aot_module = true
                result = "{warnings}{run_aot(program.get_ptr(), ctx.get_ptr(), cop)}"
            }
            return result
        }
    }

    // single function AOT
    return compile_and_simulate_ctx(file, project) <| $(program : smart_ptr<Program>; var ctx : smart_ptr<Context>; issues : string) {
        let all_funcs <- collect_aot_functions(program)

        // try exact match on fn.name first, then on aot_name
        var matches : array<int>
        for (i in range(length(all_funcs))) {
            if (all_funcs[i].name == func_name) {
                matches |> push(i)
            }
        }
        // exact match on aot_name (mangled C++ name)
        if (empty(matches)) {
            for (i in range(length(all_funcs))) {
                if (all_funcs[i].aot_name == func_name) {
                    matches |> push(i)
                }
            }
        }
        // fuzzy: method match and generic origin
        if (empty(matches)) {
            for (i in range(length(all_funcs))) {
                if (match_function_name(all_funcs[i].name, func_name) || match_function_generic(all_funcs[i].name, func_name, program)) {
                    matches |> push(i)
                }
            }
        }

        if (empty(matches)) {
            var sigs : array<string>
            for (fi in all_funcs) {
                sigs |> push(fi.signature)
            }
            return "No function matching '{func_name}' found.\nAvailable functions:\n" + join(sigs, "\n")
        }
        if (length(matches) == 1) {
            let matched_name = all_funcs[matches[0]].aot_name
            var result : string
            using() <| $(var cop : CodeOfPolicies) {
                cop.aot = false
                cop.aot_module = true
                result = run_aot_function(program.get_ptr(), ctx.get_ptr(), cop, matched_name)
            }
            if (empty(result)) {
                return "Function '{matched_name}' produced no AOT output (marked noAot or no_aot)."
            }
            return result
        }
        // multiple matches — show signatures with aot_name for disambiguation
        var lines : array<string>
        for (idx in matches) {
            lines |> push("{all_funcs[idx].signature}  [{all_funcs[idx].aot_name}]")
        }
        return "Multiple functions match '{func_name}':\n" + join(lines, "\n") + "\nUse the bracketed name for exact selection."
    }
}
