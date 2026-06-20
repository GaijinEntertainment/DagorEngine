options gen2
options rtti
options indenting = 4

require ../tools/common.das public

//! Subprocess form of `list_types`. Argv: <file> <project>.

def write_type_list(var writer : StringBuilderWriter; program : smart_ptr<Program>) {
    let thisMod = get_this_module(program)
    var structs : array<string>
    var classes : array<string>
    var enums : array<string>
    var typedefs : array<string>
    for_each_structure(thisMod) $(value) {
        if (value.flags.privateStructure || value.flags.isLambda || value.flags.generated || value.flags._generator) return
        let entry = build_string() $(var w) {
            if (value.flags.isClass) {
                write(w, "  class {value.name}")
                if (value.parent != null) {
                    write(w, " : {value.parent.name}")
                }
                write(w, "  // line {int(value.at.line)}\n")
                for (fld in value.fields) {
                    if (fld.flags.classMethod || fld.flags.generated || !fld.flags.implemented || fld.flags.parentType || fld.name == "__rtti") continue
                    write(w, "    {fld.name} : {describe(fld._type)}")
                    if (fld.init != null) {
                        write(w, " = {describe(fld.init)}")
                    }
                    write(w, "\n")
                }
            } else {
                write(w, "  struct {value.name}")
                if (value.parent != null) {
                    write(w, " : {value.parent.name}")
                }
                write(w, "  // line {int(value.at.line)}\n")
                for (fld in value.fields) {
                    if (fld.flags.generated || !fld.flags.implemented || fld.flags.parentType || fld.name == "__rtti") continue
                    write(w, "    {fld.name} : {describe(fld._type)}")
                    if (fld.init != null) {
                        write(w, " = {describe(fld.init)}")
                    }
                    write(w, "\n")
                }
            }
        }
        if (value.flags.isClass) {
            classes |> push(entry)
        } else {
            structs |> push(entry)
        }
    }
    for_each_enumeration(thisMod) $(value) {
        if (value.isPrivate) return
        let entry = build_string() $(var w) {
            write(w, "  enum {value.name}  // line {int(value.at.line)}\n")
            for (en in value.list) {
                write(w, "    {en.name} = {describe(en.value)}\n")
            }
        }
        enums |> push(entry)
    }
    for_each_typedef(thisMod) $(name, value) {
        if (value.flags.isPrivateAlias) return
        typedefs |> push("  {name} = {describe(value)}  // line {int(value.at.line)}\n")
    }
    if (!empty(structs)) {
        write(writer, "Structs:\n")
        for (s in structs) {
            write(writer, s)
        }
    }
    if (!empty(classes)) {
        write(writer, "Classes:\n")
        for (c in classes) {
            write(writer, c)
        }
    }
    if (!empty(enums)) {
        write(writer, "Enumerations:\n")
        for (e in enums) {
            write(writer, e)
        }
    }
    if (!empty(typedefs)) {
        write(writer, "Type aliases:\n")
        for (t in typedefs) {
            write(writer, t)
        }
    }
}

def run_list_types(file : string; project : string = "") : string {
    return compile_and_simulate(file, project) $(program; _issues) {
        let res = build_string() $(var writer) {
            write_type_list(writer, program)
        }
        return empty(res) ? "(no types found)" : res
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 2) {
        print(make_tool_result("list_types subtool: expected 2 args (file, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let project = args[1]
    print(run_list_types(file, project))
}
