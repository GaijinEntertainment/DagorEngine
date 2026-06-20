options gen2
options rtti
options indenting = 4

require ../tools/common.das public

//! Subprocess form of `describe_type`. Argv: <name> <module_name> <project>.

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
    if (!empty(value.fields)) {
        write(w, "Fields:\n")
        for (fld in value.fields) {
            write(w, "  {fld.name} : {describe(fld._type)}")
            if (fld.init != null) {
                write(w, "  // has default")
            }
            write(w, "\n")
        }
    }
    var methods : array<string>
    for_each_function(mod, "") $(func) {
        if (!func.flags.isClassMethod || empty(func.arguments)) return
        let self_type = func.arguments[0]._type
        if (self_type == null || self_type.structType == null ||
                self_type.structType.name != value.name ||
                (func.flags.generated && func.fromGeneric == null)) return
        let entry = build_string() $(var ew) {
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
    basic_struct_for_each_parent(*ann) $(parent) {
        write(w, " : {parent.name}")
    }
    write(w, "  // module {mod.name}")
    if (!empty(value.cppName)) {
        write(w, ", C++ {value.cppName}")
    }
    write(w, "\n")
    var field_count = 0
    for_each_field(*ann) $(fname, cppName, xtype, _offset) {
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
    var methods : array<string>
    for_each_function(mod, "") $(func) {
        if (func.flags.privateFunction || func.flags._lambda || func.flags.generated || empty(func.arguments)) return
        let first_type = func.arguments[0]._type
        if (first_type == null) return
        if (first_type.baseType == Type.tHandle) {
            if (first_type.annotation != null && first_type.annotation.name == value.name) {
                let entry = build_string() $(var ew) {
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
    let typ = td
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

def private try_describe_type(name, module_name, preamble, project : string) : TryResult {
    let stub_path = make_temp_das_file()
    var stub = "options gen2\n"
    if (!empty(module_name)) {
        stub = "{stub}require {module_name}\n"
    }
    stub = "{stub}[export]\ndef main() \{\}\n"
    var write_ok = false
    fopen(stub_path, "w") $(f) {
        if (f != null) {
            fwrite(f, stub)
            write_ok = true
        }
    }
    if (!write_ok) {
        remove(stub_path)
        return TryResult(text = make_tool_result("Cannot write temp file: {stub_path}", true), ok = false, retryable = false)
    }
    var ok = false
    let res = compile_and_simulate(stub_path, project) $(program; _issues) {
        ok = true
        var result = ""
        program_for_each_module(program) $(mod) {
            if (!empty(result)) return
            for_each_structure(mod) $(value) {
                if (!empty(result) || value.name != name ||
                        value.flags.isLambda || value.flags._generator || value.flags.generated) return
                result = build_string() $(var w) {
                    write_struct_detail(w, value, mod)
                }
            }
            if (empty(result)) {
                module_for_each_annotation(mod) $(value) {
                    if (!empty(result) || !value.isBasicStructureAnnotation || value.name != name) return
                    result = build_string() $(var w) {
                        write_handled_type_detail(w, value, mod)
                    }
                }
            }
            if (empty(result)) {
                for_each_enumeration(mod) $(value) {
                    if (!empty(result) || value.name != name) return
                    result = build_string() $(var w) {
                        write_enum_detail(w, value, mod)
                    }
                }
            }
            if (empty(result)) {
                for_each_typedef(mod) $(tname, td) {
                    if (!empty(result) || tname != name) return
                    result = build_string() $(var w) {
                        write_typedef_detail(w, string(tname), td, mod)
                    }
                }
            }
        }
        if (empty(result)) return "{preamble}Type '{name}' not found"
        return "{preamble}{result}"
    }
    remove(stub_path)
    return TryResult(text = res, ok = ok, retryable = true)
}

def run_describe_type(name, module_name : string; project : string = "") : string {
    if (empty(name)) return make_tool_result("missing 'name' argument", true)
    let first = try_describe_type(name, module_name, "", project)
    if (first.ok || !first.retryable || empty(module_name) || is_qualified_module_name(module_name)) return first.text
    let resolved = "daslib/{module_name}"
    let preamble = "[resolved '{module_name}' as '{resolved}']\n"
    let second = try_describe_type(name, resolved, preamble, project)
    if (second.ok) return second.text
    return first.text
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 3) {
        print(make_tool_result("describe_type subtool: expected 3 args (name, module_name, project), got {length(args)}", true))
        return
    }
    let name = args[0]
    let module_name = args[1]
    let project = args[2]
    print(run_describe_type(name, module_name, project))
}
