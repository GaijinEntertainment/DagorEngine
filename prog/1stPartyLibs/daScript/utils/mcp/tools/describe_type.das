options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public

def do_describe_type(name, module_name : string; project : string = "") : string {
    if (empty(name)) {
        return make_tool_result("missing 'name' argument", true)
    }
    // build stub that requires the module (if specified)
    let stub_path = make_temp_das_file()
    var stub = "options gen2\n"
    if (!empty(module_name)) {
        stub = "{stub}require {module_name}\n"
    }
    stub = "{stub}[export]\ndef main() \{\}\n"
    fopen(stub_path, "w") <| $(f) {
        fwrite(f, stub)
    }
    let res = compile_and_simulate(stub_path, project) <| $(program; issues) {
        var result = ""
        // search all modules for the type
        program_for_each_module(program) <| $(mod) {
            if (!empty(result)) {
                return
            }
            // structs and classes
            for_each_structure(mod) <| $(value) {
                if (!empty(result)) {
                    return
                }
                if (string(value.name) != name) {
                    return
                }
                if (value.flags.isLambda || value.flags._generator || value.flags.generated) {
                    return
                }
                result = build_string() <| $(var w) {
                    write_struct_detail(w, value, mod)
                }
            }
            // handled types
            if (empty(result)) {
                module_for_each_annotation(mod) <| $(value) {
                    if (!empty(result)) {
                        return
                    }
                    if (!value.isBasicStructureAnnotation) {
                        return
                    }
                    if (string(value.name) != name) {
                        return
                    }
                    result = build_string() <| $(var w) {
                        write_handled_type_detail(w, value, mod)
                    }
                }
            }
            // enumerations
            if (empty(result)) {
                for_each_enumeration(mod) <| $(value) {
                    if (!empty(result)) {
                        return
                    }
                    if (string(value.name) != name) {
                        return
                    }
                    result = build_string() <| $(var w) {
                        write_enum_detail(w, value, mod)
                    }
                }
            }
            // typedefs (bitfields, variants, tuples, aliases)
            if (empty(result)) {
                for_each_typedef(mod) <| $(tname, td) {
                    if (!empty(result)) {
                        return
                    }
                    if (string(tname) != name) {
                        return
                    }
                    result = build_string() <| $(var w) {
                        write_typedef_detail(w, string(tname), td, mod)
                    }
                }
            }
        }
        if (empty(result)) {
            return "Type '{name}' not found"
        }
        return result
    }
    remove(stub_path)
    return res
}

def write_struct_detail(var w : StringBuilderWriter; value; mod : Module?) {
    if (value.flags.isClass) {
        write(w, "class {value.name}")
    } else {
        write(w, "struct {value.name}")
    }
    if (value.parent != null) {
        write(w, " : {value.parent.name}")
    }
    write(w, "  // module {mod.name}")
    if (int(value.at.line) > 0) {
        write(w, ", line {int(value.at.line)}")
    }
    write(w, "\n")
    // fields
    if (length(value.fields) > 0) {
        write(w, "Fields:\n")
        for (fld in value.fields) {
            write(w, "  {fld.name} : {describe(fld._type)}")
            if (fld.init != null) {
                write(w, "  // has default")
            }
            write(w, "\n")
        }
    }
    // methods
    var methods : array<string>
    for_each_function(mod, "") <| $(func) {
        if (!func.flags.isClassMethod) {
            return
        }
        if (length(func.arguments) == 0) {
            return
        }
        var self_type = get_ptr(func.arguments[0]._type)
        if (self_type == null || self_type.structType == null) {
            return
        }
        if (string(self_type.structType.name) != string(value.name)) {
            return
        }
        if (func.flags.generated && func.fromGeneric == null) {
            return
        }
        var entry = build_string() <| $(var ew) {
            write(ew, "  def {func.name}")
            write_func_signature(ew, func)
        }
        methods |> push(entry)
    }
    if (!empty(methods)) {
        write(w, "Methods:\n")
        for (m in methods) {
            write(w, "{m}\n")
        }
    }
}

def write_handled_type_detail(var w : StringBuilderWriter; value : Annotation const; mod : Module?) {
    write(w, "handled {value.name}")
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)
    basic_struct_for_each_parent(*ann) <| $(parent) {
        write(w, " : {parent.name}")
    }
    write(w, "  // module {mod.name}")
    if (!empty(value.cppName)) {
        write(w, ", C++ {value.cppName}")
    }
    write(w, "\n")
    var field_count = 0
    for_each_field(*ann) <| $(fname, cppName, xtype, offset) {
        if (field_count == 0) {
            write(w, "Fields:\n")
        }
        field_count++
        write(w, "  {fname} : {describe(xtype)}")
        if (!empty(cppName) && cppName != fname) {
            write(w, "  // C++ {cppName}")
        }
        write(w, "\n")
    }
    // methods on handled types — functions where first arg matches this type
    var methods : array<string>
    for_each_function(mod, "") <| $(func) {
        if (func.flags.privateFunction || func.flags._lambda || func.flags.generated) {
            return
        }
        if (length(func.arguments) == 0) {
            return
        }
        var first_type = get_ptr(func.arguments[0]._type)
        if (first_type == null) {
            return
        }
        // check if first arg is this handled type (possibly as pointer)
        if (first_type.baseType == Type.tHandle) {
            if (first_type.annotation != null && string(first_type.annotation.name) == string(value.name)) {
                var entry = build_string() <| $(var ew) {
                    write(ew, "  def {func.name}")
                    write_func_signature(ew, func)
                }
                methods |> push(entry)
            }
        }
    }
    if (!empty(methods)) {
        write(w, "Functions (first arg is {value.name}):\n")
        for (m in methods) {
            write(w, "{m}\n")
        }
    }
}

def write_enum_detail(var w : StringBuilderWriter; value; mod : Module?) {
    write(w, "enum {value.name}  // module {mod.name}")
    if (int(value.at.line) > 0) {
        write(w, ", line {int(value.at.line)}")
    }
    write(w, "\n")
    write(w, "Values:\n")
    for (en in value.list) {
        write(w, "  {en.name}")
        if (en.value != null) {
            write(w, " = {describe(en.value)}")
        }
        write(w, "\n")
    }
}

def write_typedef_detail(var w : StringBuilderWriter; name : string; td; mod : Module?) {
    let typ = get_ptr(td)
    if (typ.baseType == Type.tBitfield) {
        write(w, "bitfield {name}  // module {mod.name}\n")
        write(w, "Flags:\n")
        for (an in typ.argNames) {
            write(w, "  {an}\n")
        }
    } elif (typ.baseType == Type.tVariant) {
        write(w, "variant {name}  // module {mod.name}\n")
        write(w, "Options:\n")
        for (an, at in typ.argNames, typ.argTypes) {
            write(w, "  {an} : {describe(at)}\n")
        }
    } elif (typ.baseType == Type.tTuple) {
        write(w, "tuple {name}  // module {mod.name}\n")
        write(w, "Fields:\n")
        for (at, idx in typ.argTypes, count()) {
            var fname = "_{idx}"
            if (idx < length(typ.argNames)) {
                let n = string(typ.argNames[idx])
                if (!empty(n)) {
                    fname = n
                }
            }
            write(w, "  {fname} : {describe(at)}\n")
        }
    } else {
        write(w, "typedef {name} = {describe(td)}  // module {mod.name}\n")
    }
}
