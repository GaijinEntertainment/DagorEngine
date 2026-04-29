options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public

def do_list_module_api(module_name : string; filter : string = ""; section : string = ""; compact_str : string = ""; project : string = "") : string {
    let compact = compact_str == "true"
    // write a temp stub file that requires the module
    let stub_path = make_temp_das_file()
    let stub = "require {module_name}\n[export]\ndef main\n    pass\n"
    fopen(stub_path, "w") <| $(f) {
        fwrite(f, stub)
    }
    let result = compile_and_simulate(stub_path, project) <| $(program; issues) {
        // find the target module
        var target_mod : Module?
        program_for_each_module(program) <| $(mod) {
            let mname = string(mod.name)
            if (mname == module_name || "{module_name}" |> ends_with("/{mname}")) {
                target_mod = mod
            }
        }
        if (target_mod == null) {
            return "Module '{module_name}' not found after compilation"
        }
        var res = build_string() <| $(var writer) {
            write_module_api(writer, target_mod, filter, section, compact)
        }
        return empty(res) ? "(empty module)" : res
    }
    remove(stub_path)
    return result
}

def is_operator_function(name : string) : bool {
    let ch = first_character(name)
    return !is_alpha(ch) && !is_number(ch) && ch != '_'
}

def name_matches_filter(name : string; filter : string) : bool {
    return empty(filter) || symbol_matches(name, filter)
}

def want_section(section : string; name : string) : bool {
    return empty(section) || section == name
}

def write_compact_func_signature(var w : StringBuilderWriter; func) {
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
        write(w, "{describe(func.arguments[i]._type)}")
    }
    if (has_args) {
        write(w, ")")
    }
    if (func.result != null && !func.result.isVoid) {
        write(w, " : {describe(func.result)}")
    }
}

def write_module_api(var writer : StringBuilderWriter; mod : Module?; filter : string = ""; section : string = ""; compact : bool = false) {
    write(writer, "Module: {mod.name}\n")
    var funcs : array<string>
    var generics : array<string>
    var operators : array<string>
    var structs : array<string>
    var handled : array<string>
    var enums : array<string>
    var globals : array<string>
    var func_annotations : array<string>
    var struct_annotations : array<string>
    var call_macros : array<string>
    var reader_macros : array<string>
    var variant_macros : array<string>
    var typeinfo_macros : array<string>
    var for_loop_macros : array<string>
    var type_macros : array<string>
    // functions via AST
    if (want_section(section, "functions") || want_section(section, "operators")) {
        for_each_function(mod, "") <| $(func) {
            if (func.flags.privateFunction || func.flags._lambda || func.flags.generated) {
                return
            }
            let fname = string(func.name)
            if (!name_matches_filter(fname, filter)) {
                return
            }
            var entry = build_string() <| $(var w) {
                write(w, "  def {func.name}")
                if (compact) {
                    write_compact_func_signature(w, func)
                } else {
                    write_func_signature(w, func)
                }
            }
            if (is_operator_function(fname) && want_section(section, "operators")) {
                operators |> push(entry)
            } elif (!is_operator_function(fname) && want_section(section, "functions")) {
                funcs |> push(entry)
            }
        }
    }
    // generics via AST
    if (want_section(section, "generics") || want_section(section, "operators")) {
        for_each_generic(mod) <| $(func) {
            if (func.flags.privateFunction || func.flags.generated || string(func.name) |> starts_with("```")) {
                return
            }
            let fname = string(func.name)
            if (!name_matches_filter(fname, filter)) {
                return
            }
            var entry = build_string() <| $(var w) {
                write(w, "  def {func.name}")
                if (compact) {
                    write_compact_func_signature(w, func)
                } else {
                    write_func_signature(w, func)
                }
            }
            if (is_operator_function(fname) && want_section(section, "operators")) {
                operators |> push(entry)
            } elif (!is_operator_function(fname) && want_section(section, "generics")) {
                generics |> push(entry)
            }
        }
    }
    // structures via AST
    if (want_section(section, "structs")) {
        for_each_structure(mod) <| $(value) {
        if (value.flags.isLambda || value.flags._generator || value.flags.generated || value.flags.privateStructure) {
            return
        }
        if (!name_matches_filter(string(value.name), filter)) {
            return
        }
        var entry = build_string() <| $(var w) {
            if (value.flags.isClass) {
                write(w, "  class {value.name}")
            } else {
                write(w, "  struct {value.name}")
            }
            if (value.parent != null) {
                write(w, " : {value.parent.name}")
            }
            if (!compact && int(value.at.line) > 0) {
                write(w, "  // line {int(value.at.line)}")
            }
            write(w, "\n")
            if (!compact) {
                for (fld in value.fields) {
                    write(w, "    {fld.name} : {describe(fld._type)}\n")
                }
            }
        }
        structs |> push(entry)
        }
    }
    // handled types via annotations
    if (want_section(section, "handled")) {
        module_for_each_annotation(mod) <| $(value) {
            if (!value.isBasicStructureAnnotation) {
                return
            }
            if (!name_matches_filter(string(value.name), filter)) {
                return
            }
            var entry = build_string() <| $(var w) {
                if (compact) {
                    write(w, "  handled {value.name}\n")
                } else {
                    write_handled_type(w, value)
                }
            }
            handled |> push(entry)
        }
    }
    // enumerations via AST
    if (want_section(section, "enums")) {
        for_each_enumeration(mod) <| $(value) {
            if (value.isPrivate) {
                return
            }
            if (!name_matches_filter(string(value.name), filter)) {
                return
            }
            var entry = build_string() <| $(var w) {
                write(w, "  enum {value.name}")
                if (!compact && int(value.at.line) > 0) {
                    write(w, "  // line {int(value.at.line)}")
                }
                write(w, "\n")
                if (!compact) {
                    for (en in value.list) {
                        write(w, "    {en.name}\n")
                    }
                }
            }
            enums |> push(entry)
        }
    }
    // annotations (function annotations, structure macros)
    if (want_section(section, "annotations")) {
        module_for_each_annotation(mod) <| $(value) {
            if (value.isBasicStructureAnnotation || value.isEnumerationAnnotation) {
                return  // already in handled/enums sections
            }
            let aname = string(value.name)
            if (!name_matches_filter(aname, filter)) {
                return
            }
            if (value.isFunctionAnnotation) {
                func_annotations |> push("  [{aname}]\n")
            } elif (!value.isTypeAnnotation) {
                struct_annotations |> push("  [{aname}]\n")
            }
        }
    }
    // call macros
    if (want_section(section, "annotations")) {
        for_each_call_macro(mod) <| $(value) {
            let mname = string(value)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            call_macros |> push("  {mname}\n")
        }
    }
    // reader macros
    if (want_section(section, "annotations")) {
        for_each_reader_macro(mod) <| $(value) {
            let mname = string(value)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            reader_macros |> push("  {mname}\n")
        }
    }
    // variant macros
    if (want_section(section, "annotations")) {
        for_each_variant_macro(mod) <| $(value) {
            let mname = string(value.name)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            variant_macros |> push("  {mname}\n")
        }
    }
    // typeinfo macros
    if (want_section(section, "annotations")) {
        for_each_typeinfo_macro(mod) <| $(value) {
            let mname = string(value.name)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            typeinfo_macros |> push("  {mname}\n")
        }
    }
    // for loop macros
    if (want_section(section, "annotations")) {
        for_each_for_loop_macro(mod) <| $(value) {
            let mname = string(value.name)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            for_loop_macros |> push("  {mname}\n")
        }
    }
    // type macros
    if (want_section(section, "annotations")) {
        for_each_typemacro(mod) <| $(value) {
            let mname = string(value.name)
            if (!name_matches_filter(mname, filter)) {
                return
            }
            type_macros |> push("  {mname}\n")
        }
    }
    // globals via AST
    if (want_section(section, "globals")) {
        for_each_global(mod) <| $(value) {
            if (value.flags.private_variable) {
                return
            }
            if (!name_matches_filter(string(value.name), filter)) {
                return
            }
            var line = "  {value.name} : {describe(value._type)}"
            if (int(value.at.line) > 0) {
                line = "{line}  // line {int(value.at.line)}"
            }
            globals |> push("{line}\n")
        }
    }
    if (!empty(funcs)) {
        write(writer, "Functions ({length(funcs)}):\n")
        for (f in funcs) {
            write(writer, "{f}\n")
        }
    }
    if (!empty(generics)) {
        write(writer, "Generics ({length(generics)}):\n")
        for (g in generics) {
            write(writer, "{g}\n")
        }
    }
    if (!empty(operators)) {
        write(writer, "Operators ({length(operators)}):\n")
        for (o in operators) {
            write(writer, "{o}\n")
        }
    }
    if (!empty(structs)) {
        write(writer, "Structs/Classes:\n")
        for (s in structs) {
            write(writer, s)
        }
    }
    if (!empty(handled)) {
        write(writer, "Handled types ({length(handled)}):\n")
        for (h in handled) {
            write(writer, h)
        }
    }
    if (!empty(enums)) {
        write(writer, "Enumerations:\n")
        for (e in enums) {
            write(writer, e)
        }
    }
    if (!empty(globals)) {
        write(writer, "Globals:\n")
        for (g in globals) {
            write(writer, g)
        }
    }
    if (!empty(func_annotations)) {
        write(writer, "Function annotations ({length(func_annotations)}):\n")
        for (a in func_annotations) {
            write(writer, a)
        }
    }
    if (!empty(struct_annotations)) {
        write(writer, "Structure annotations ({length(struct_annotations)}):\n")
        for (a in struct_annotations) {
            write(writer, a)
        }
    }
    if (!empty(call_macros)) {
        write(writer, "Call macros ({length(call_macros)}):\n")
        for (m in call_macros) {
            write(writer, m)
        }
    }
    if (!empty(reader_macros)) {
        write(writer, "Reader macros ({length(reader_macros)}):\n")
        for (m in reader_macros) {
            write(writer, m)
        }
    }
    if (!empty(variant_macros)) {
        write(writer, "Variant macros ({length(variant_macros)}):\n")
        for (m in variant_macros) {
            write(writer, m)
        }
    }
    if (!empty(typeinfo_macros)) {
        write(writer, "Typeinfo macros ({length(typeinfo_macros)}):\n")
        for (m in typeinfo_macros) {
            write(writer, m)
        }
    }
    if (!empty(for_loop_macros)) {
        write(writer, "For-loop macros ({length(for_loop_macros)}):\n")
        for (m in for_loop_macros) {
            write(writer, m)
        }
    }
    if (!empty(type_macros)) {
        write(writer, "Type macros ({length(type_macros)}):\n")
        for (m in type_macros) {
            write(writer, m)
        }
    }
}
