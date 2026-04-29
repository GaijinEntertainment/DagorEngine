options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module rst shared private

//! RST documentation generator.
//!
//! Core engine for producing reStructuredText files from compiled
//! module RTTI. Provides ``document``, ``group_by_regex``,
//! ``describe_type``, and other helpers used by ``das2rst``.

require daslib/fio
require math
require daslib/rtti
require daslib/ast
require strings
require daslib/ast_boost
require daslib/strings_boost
require daslib/regex
require daslib/regex_boost

let { //! Configuration constants for the RST documentation generator.
    log_documentation = false
    show_hidden_groups = false
    STUB = "// stub"
}

var public {
    das_root = get_das_root()
    topic_root = "{das_root}/doc/source/stdlib/generated"
    handmade_root = "{das_root}/doc/source/stdlib"
    add_stub_header = true
    add_empty_topic = true
    strict_struct_fields = true
    group_function_overloads = true
}



def PANIC(message : string) {
    to_log(LOG_ERROR, "PANIC: {message}\n")
    unsafe(exit(-1))
}

let module_name_replacements = [
    ("$", "builtin"),
    ("ast_core", "ast"),
    ("fio_core", "fio"),
    ("network_core", "network"),
    ("rtti_core", "rtti"),
]

def module_name(mod) {
    let mn = string(mod.name)
    for ((src, dst) in module_name_replacements) {
        if (mn == src) {
            return dst
        }
    }
    return mn
}

def alias_label_name(td : TypeDeclPtr) {
    return "alias-{td.alias}"
}

def global_label_name(value : Variable?) {
    return "global-{module_name(value._module)}-{value.name}"
}

def global_label_name(value : VariablePtr) {
    return global_label_name(get_ptr(value))
}

def enum_label_name(value : Enumeration?) {
    return "enum-{module_name(value._module)}-{value.name}"
}

def enum_label_name(value : EnumerationPtr) {
    return enum_label_name(get_ptr(value))
}

def struct_label_name(value : Structure?) {
    return "struct-{module_name(value._module)}-{value.name}"
}

def struct_label_name(value : StructurePtr) {
    return struct_label_name(get_ptr(value))
}

def handle_label_name(value) {
    return "handle-{module_name(value._module)}-{value.name}"
}

def call_macro_label_name(mod, value) {
    return "call-macro-{module_name(mod)}-{value}"
}

def public safe_function_name(name : string) : string {
    //! Replaces special characters in a function name to produce a safe RST label identifier.
    return name |> replace_multiple([
        (" ", "_"),
        ("$", "_builtin_"),
        ("#", "_hh_"),
        (":", "_c_"),
        ("@", "_at_"),
        ("<", "_ls_"),
        (">", "_gr_"),
        ("`", "_rq_"),
        ("!", "_ex_"),
        ("=", "_eq_"),
        ("*", "_st_"),
        (".", "_dot_"),
        ("?", "_q_"),
        ("&", "_ref_"),
        ("(", ""),
        (")", ""),
        ("[", "_lb_"),
        ("]", "_rb_"),
        ("|", "")])
}

def public function_label_file(value : FunctionPtr | Function?; drop_args : int = 0) {
    //! Generates a unique RST label for a function based on its name and argument types.
    var mn = build_string() <| $(var w) {
        w |> write(value.name)
        var drop = drop_args
        for (arg in value.arguments) {
            if (drop-- > 0) {
                continue
            }
            let prefix = arg._type.flags.removeConstant ? "var " : ""
            if (arg._type.baseType == Type.autoinfer && arg.init != null) {
                // During rst_comment (pre-inference), auto-inferred default arg types
                // are unresolved. Determine the type from the init expression so the
                // hash matches what das2rst (post-inference) produces.
                let rtti = "{arg.init.__rtti}"
                if (rtti == "ExprConstDouble") {
                    w |> write("{prefix}double")
                } elif (rtti == "ExprConstFloat") {
                    w |> write("{prefix}float")
                } elif (rtti == "ExprConstString") {
                    w |> write("{prefix}string")
                } elif (rtti == "ExprConstBool") {
                    w |> write("{prefix}bool")
                } elif (rtti == "ExprConstInt") {
                    w |> write("{prefix}int")
                } elif (rtti == "ExprConstUInt") {
                    w |> write("{prefix}uint")
                } elif (rtti == "ExprConstInt64") {
                    w |> write("{prefix}int64")
                } elif (rtti == "ExprConstUInt64") {
                    w |> write("{prefix}uint64")
                } elif (rtti == "ExprConstInt8") {
                    w |> write("{prefix}int8")
                } elif (rtti == "ExprConstUInt8") {
                    w |> write("{prefix}uint8")
                } elif (rtti == "ExprConstInt16") {
                    w |> write("{prefix}int16")
                } elif (rtti == "ExprConstUInt16") {
                    w |> write("{prefix}uint16")
                } else {
                    w |> write("{prefix}auto")
                }
            } else {
                w |> write("{prefix}{describe_type_short(arg._type)}")
            }
        }
    }
    return "{safe_function_name(string(value.name))}-{hash(mn)}"
}

def public function_label_file(name; value : smart_ptr<TypeDecl>&; drop_args : int = 0) {
    //! Generates a unique RST label for a function type based on its name and argument types.
    var mn = build_string() <| $(var w) {
        w |> write(name)
        var drop = drop_args
        for (arg in value.argTypes) {
            if (drop-- > 0) {
                continue
            }
            let prefix = arg.flags.removeConstant ? "var " : ""
            w |> write("{prefix}{describe_type_short(arg)}")
        }
    }
    return "{safe_function_name(string(name))}-{hash(mn)}"
}

def function_short_mangled_name(value : FunctionPtr | Function?) {
    return build_string() <| $(var w) {
        if (value._module != null) {
            w |> write(module_name(value._module))
            w |> write("_")
        }
        w |> write(value.name)
        for (arg in value.arguments) {
            if (arg._type.baseType == Type.fakeContext || arg._type.baseType == Type.fakeLineInfo) {
                continue
            }
            w |> write("_")
            if (arg._type.flags.removeConstant && arg._type.flags.explicitConst) {
                w |> write("_")
            }
            w |> write(describe_type_short(arg._type))
        }
    }
}

def function_label_name(value : FunctionPtr | Function?) {
    let mn = function_short_mangled_name(value)
    let res = "function-{safe_function_name(mn)}"
    // For generic functions, append a line suffix to disambiguate overloads
    // that have the same short mangled name
    if (group_function_overloads) {
        var is_generic = false
        for (arg in value.arguments) {
            if (arg._type.baseType == Type.autoinfer) {
                is_generic = true
                break
            }
        }
        if (is_generic) {
            return "{res}_{value.at.line}"
        }
        // For class/struct methods, operators like []=, []&&=, []||= collapse
        // to the same safe name. Disambiguate with line number.
        if (value.flags.isClassMethod) {
            return "{res}_{value.at.line}"
        }
    }
    return res
}


def describe_type_short(td : TypeDecl?) {
    unsafe {
        return describe_type_short(reinterpret<TypeDeclPtr> td)
    }
}

def cleanup_type_short(name : string) {
    var res = name
    if (res |> ends_with(" const")) {
        res = res |> slice(0, -6)
    }
    if (res |> ends_with(" const&")) {
        res = res |> slice(0, -7)
    }
    if (res |> ends_with(" -const")) {
        res = res |> slice(0, -7)
    }
    if (res |> ends_with(" const#")) {
        res = "{res |> slice(0,-7)}#"
    }
    if (res |> ends_with(" const[]")) {
        res = "{res |> slice(0,-8)}[]"
    }
    if (res |> ends_with(" const[]#")) {
        res = "{res |> slice(0,-9)}[]#"
    }
    if (res |> ends_with(" const implicit")) {
        res = res |> slice(0, -15)
    }
    if (res |> ends_with(" const implicit#")) {
        res = "{res |> slice(0,-16)}#"
    }
    if (res |> ends_with(" implicit")) {
        res = res |> slice(0, -9)
    }
    if (res |> ends_with(" implicit#")) {
        res = "{res |> slice(0,-10)}#"
    }
    if (res |> ends_with(" const ==const")) {
        res = "{res |> slice(0,-14)} ==const"
    }
    return res
}

def describe_type_short(td : TypeDeclPtr) {
    if (td == null) {
        PANIC("describe_type_short: null type")
    }
    if (!empty(td.alias) && !td.isAuto && !td.isAlias) {
        return string(td.alias)
    }
    var res : string
    if (td.baseType == Type.tArray) {
        if (td.firstType == null) {
            res = "array<anything>"
        } else {
            res = "array<{describe_type_short(td.firstType)}>"
        }
    } elif (td.baseType == Type.tTable) {
        if (td.firstType == null || td.secondType == null) {
            res = "table<anything, anything>"
        } else {
            res = "table<{describe_type_short(td.firstType)}, {describe_type_short(td.secondType)}>"
        }
    } elif (td.baseType == Type.tFunction || td.baseType == Type.tBlock || td.baseType == Type.tLambda) {
        res = build_string() <| $(var w) {
            w |> write(td.baseType == Type.tBlock ? "block<" : td.baseType == Type.tFunction ? "function<" : "lambda<")
            if (td.argTypes |> length != 0) {
                w |> write("(")
                if (td.argNames |> length != 0) {
                    var first = true
                    for (at, an in td.argTypes, td.argNames) {
                        if (first) {
                            first = false
                        } else {
                            w |> write(";")
                        }
                        if (at.flags.removeConstant) {
                            w |> write("var ")
                        }
                        w |> write(string(an))
                        w |> write(":")
                        w |> write(describe_type_short(at))
                    }
                } else {
                    var first = true
                    for (at in td.argTypes) {
                        if (first) {
                            first = false
                        } else {
                            w |> write(";")
                        }
                        w |> write(describe_type_short(at))
                    }
                }
                w |> write(")")
                if (td.firstType != null) {
                    w |> write(":")
                    w |> write(describe_type_short(td.firstType))
                } else {
                    w |> write(":void")
                }
            } else {
                w |> write("():void")
            }
            w |> write(">")
        }
    }
    if (!empty(res)) {
        if (td.flags.temporary) {
            res += "#"
        }
        return res
    }

    let tp = describe(td, false, false, false)
    res = replace(tp, "$::", "")
    if (res |> find("|") != -1) {
        var inscope parts <- split(res, "|")
        for (p in parts) {
            p = cleanup_type_short(p)
        }
        return join(parts, "|") |> cleanup_type_short()
    }

    return cleanup_type_short(res)
}

def public describe_short(expr : Expression? | smart_ptr<Expression>&) {
    //! Returns a short description of an expression, using empty parentheses for empty make-structs.
    if (expr is ExprMakeStruct) {
        let str = expr as ExprMakeStruct
        var count = 0
        for (it in str.structs) {
            for (_ in *it) {
                count++
            }
        }
        if (count == 0) {
            var res = describe_type_short(str.makeType) + "()"
            return res
        }
    }

    return describe(expr)
}

def rst_describe_function_short(func : FunctionPtr | Function?) {
    var inscope args : array<string>
    for (x in func.arguments) {
        continue if (x._type.baseType == Type.fakeContext)
        continue if (x._type.baseType == Type.fakeLineInfo)
        let prefix = x._type.flags.removeConstant ? "var " : ""
        if (x.init != null) {
            args |> push("{prefix}{x.name}: {describe_type_short(x._type)} = {describe_short(x.init)}")
        } else {
            args |> push("{prefix}{x.name}: {describe_type_short(x._type)}")
        }
    }

    var res = "{function_name(func)} ({join(args,"; ")})"
    if (func.result != null && func.result.baseType != Type.tVoid) {
        res = "{res} : {describe_type_short(func.result)}"
    }
    return res
}

def describe_type(td : TypeDecl?) {
    unsafe {
        return describe_type(reinterpret<TypeDeclPtr> td)
    }
}

def describe_type(td : TypeDeclPtr) {
    return build_string() $(var writer) {
        if (!empty(td.alias) && !td.isAuto && !td.isAlias) {
            write(writer, make_ref(alias_label_name(td), string(td.alias)))
            return
        }
        let baseType = td.baseType
        if (baseType == Type.alias) {
            if (!empty(td.alias)) {
                write(writer, td.alias)
            } else {
                write(writer, "alias")
            }
        } elif (baseType == Type.autoinfer) {
            write(writer, "auto")
            if (!empty(td.alias)) {
                write(writer, "({td.alias})")
            }
        } elif (baseType == Type.tHandle) {
            let hname = string(td.annotation.name)
            if (hname |> starts_with("dasvector`smart_ptr`")) {// 20 characters
                write(writer, "vector<smart_ptr<")
                write(writer, hname |> slice(20))
                write(writer, ">>")
            } elif (hname |> starts_with("dasvector`")) {// 10 characters
                write(writer, "vector<")
                write(writer, hname |> slice(10))
                write(writer, ">")
            } elif (hname |> starts_with("pair`")) {// 5 characters
                write(writer, "pair<")
                var parts <- split(hname, "`")
                write(writer, parts[1])
                write(writer, ",")
                write(writer, parts[2])
                write(writer, ">")
            } else {
                write(writer, make_ref(handle_label_name(td.annotation), "{td.annotation.name}"))
            }
        } elif (baseType == Type.tStructure) {
            write(writer, make_ref(struct_label_name(td.structType), "{td.structType.name}"))
        } elif (baseType == Type.tEnumeration || baseType == Type.tEnumeration8 || baseType == Type.tEnumeration16 || baseType == Type.tEnumeration64) {
            write(writer, make_ref(enum_label_name(td.enumType), "{td.enumType.name}"))
        }
        //         write(writer,make_ref(handle_label_name(td.annotation),"{module_name(td.annotation._module)}::{td.annotation.name}"))
        // elif baseType==Type tStructure
        //     write(writer,make_ref(struct_label_name(td.structType),"{module_name(td.structType._module)}::{td.structType.name}"))
        // elif baseType==Type tEnumeration || baseType==Type tEnumeration8 || baseType==Type tEnumeration16 || baseType==Type tEnumeration64
        //     write(writer,make_ref(enum_label_name(td.enumType),"{module_name(td.enumType._module)}::{td.enumType.name}"))
        elif (baseType == Type.tPointer) {
            if (td.flags.smartPtr) {
                write(writer, "smart_ptr<")
            }
            if (td.firstType != null) {
                write(writer, describe_type(td.firstType))
            } else {
                write(writer, "void")
            }
            write(writer, td.flags.smartPtr ? ">" : "?")
        } elif (baseType == Type.tArray) {
            write(writer, "array")
            if (td.firstType != null) {
                write(writer, "<")
                write(writer, describe_type(td.firstType))
                write(writer, ">")
            }
        } elif (baseType == Type.tTable) {
            write(writer, "table")
            if (td.firstType != null) {
                write(writer, "<")
                write(writer, describe_type(td.firstType))
                write(writer, ";")
                write(writer, describe_type(td.secondType))
                write(writer, ">")
            }
        } elif (baseType == Type.tIterator) {
            write(writer, "iterator")
            if (td.firstType != null) {
                write(writer, "<")
                write(writer, describe_type(td.firstType))
                write(writer, ">")
            }
        } elif (baseType == Type.tBlock || baseType == Type.tFunction || baseType == Type.tLambda) {
            write(writer, das_to_string(baseType))
            write(writer, "<")
            if (td.argTypes |> length != 0) {
                write(writer, "(")
                if (td.argNames |> length != 0) {
                    var first = true
                    for (at, an in td.argTypes, td.argNames) {
                        if (first) {
                            first = false
                        } else {
                            write(writer, ";")
                        }
                        write(writer, an)
                        write(writer, ":")
                        write(writer, describe_type(at))
                    }
                } else {
                    var first = true
                    for (at in td.argTypes) {
                        if (first) {
                            first = false
                        } else {
                            write(writer, ";")
                        }
                        write(writer, describe_type(at))
                    }
                }
                write(writer, ")")
                if (td.firstType != null) {
                    write(writer, ":")
                    write(writer, describe_type(td.firstType))
                }
            } else {
                write(writer, "void")
            }
            write(writer, ">")
        } elif (baseType == Type.tTuple || baseType == Type.tVariant || baseType == Type.option) {
            let delim = baseType == Type.option ? "\\ | " : ";"
            write(writer, das_to_string(baseType))
            write(writer, "<")
            if (td.argTypes |> length != 0) {
                if (td.argNames |> length != 0) {
                    var first = true
                    for (at, an in td.argTypes, td.argNames) {
                        if (first) {
                            first = false
                        } else {
                            write(writer, delim)
                        }
                        write(writer, an)
                        write(writer, ":")
                        write(writer, describe_type(at))
                    }
                } else {
                    var first = true
                    for (at in td.argTypes) {
                        if (first) {
                            first = false
                        } else {
                            write(writer, delim)
                        }
                        write(writer, describe_type(at))
                    }
                }
            }
            write(writer, ">")
        } elif (baseType == Type.tBitfield || baseType == Type.tBitfield8 || baseType == Type.tBitfield16 || baseType == Type.tBitfield64) {
            write(writer, "bitfield")
            if (baseType == Type.tBitfield8) {
                write(writer, " : uint8")
            } elif (baseType == Type.tBitfield16) {
                write(writer, " : uint16")
            } elif (baseType == Type.tBitfield64) {
                write(writer, " : uint64")
            }
            write(writer, "<")
            var first = true
            for (an in td.argNames) {
                if (first) {
                    first = false
                } else {
                    write(writer, ";")
                }
                write(writer, an)
            }
            write(writer, ">")
        } else {
            write(writer, das_to_string(baseType))
        }
        // if td.flags.constant
        //     write(writer," const")
        for (d in td.dim) {
            write(writer, "[")
            write(writer, d)
            write(writer, "]")
        }
        if (td.flags.ref) {
            write(writer, "\\ &")
        }
        if (td.flags.temporary) {
            write(writer, "\\ #")
        }
        if (td.flags._implicit) {
            write(writer, " implicit")
        }
        if (td.flags.explicitConst) {
            write(writer, "!")
        }
    }
}

def make_domain(attr : string) {
    return ".. {attr}:: "
}

def make_label(name : string) {
    return ".. _{name}:\n\n"
}

def make_ref(name, text : string) {
    let t = (text |> replace_multiple([
        ("<", "\\<"),
        (">", "\\>"),
        ("!", "\\!"
    )]))
    return " :ref:`{t} <{name}>`"
}

def public make_group(name : string; plus : string = "+") {
    //! Creates an RST group header with the given name surrounded by separator lines.
    let len = length(name)
    return "\n{strings::repeat(plus,len)}\n{name}\n{strings::repeat(plus,len)}\n\n"
}

def make_header(name, lab : string) {
    let wide = length(name)
    return ".. _stdlib_{lab}:\n\n{strings::repeat("=",wide)}\n{name}\n{strings::repeat("=",wide)}\n\n"
}

def private write_table_separator(var writer : StringBuilderWriter; wid : array<int>; sep : string = "-") {
    for (x in wid) {
        write(writer, "+{strings::repeat(sep,x)}")
    }
    write(writer, "+\n")
}

def private write_table_line(var writer : StringBuilderWriter; line : array<string>; wid : array<int>) {
    for (x, l in wid, line) {
        write(writer, "+{wide(l,x)}")
    }
    write(writer, "+\n")
}

def make_table(title : string; tab : array<array<string>>) {
    return build_string() $(writer) {
        let header = ":{title}:"
        let headerLen = length(header)
        let offset = strings::repeat(" ", headerLen)
        write(writer, header)
        for (idx in range(1, length(tab))) {

            var first = true
            let itemsNum = length(tab[idx])
            for (i, l in count(), tab[idx]) {
                if (first) {
                    first = false
                    let off = idx == 1 ? "" : offset
                    write(writer, "{off} * **{l}**")
                    continue
                }
                let last = i == itemsNum - 1

                if (l != "") {
                    if (tab[0][i] == "value") {
                        write(writer, " = {l}")
                    } elif (tab[0][i] == "description") {
                        write(writer, " - {l}")
                    } elif (tab[0][i] == "type") {
                        write(writer, " : {l}")
                    } else {
                        write(writer, " ({l})")
                    }
                }

                if (last) {
                    write(writer, "\n\n")
                }
            }
        }
    }
}



def make_table_(tab : array<array<string>>; withHeader : bool = false) {
    return build_string() $(writer) {
        let rows = length(tab)
        var cols = 0
        for (c in tab) {
            cols = max(cols, length(c))
        }
        var wid : array<int>
        resize(wid, cols)
        for (cc in tab) {
            for (x in range(cols)) {
                wid[x] = max(wid[x], length(cc[x]))
            }
        }
        for (ccc, idx in tab, count()) {
            write_table_separator(writer, wid, (idx == 1 && withHeader) ? "=" : "-")
            write_table_line(writer, ccc, wid)
        }
        write_table_separator(writer, wid)
        write(writer, "\n\n")
    }
}

// def make_fake_table(tab:array<array<string>>)
//     return build_string() $(writer)
//         let rows = length(tab)
//         var cols = 0
//         for c in tab
//             cols = max(cols,length(c))
//         var wid : array<int>
//         resize(wid,cols)
//         for cc in tab
//             for x in range(cols)
//                 wid[x] = max(wid[x], length(cc[x]))
//         for ccc in tab
//             for x,l in wid,ccc
//                 write(writer,"+{strings::repeat(\"-\",l)}")
//             write(writer,"+\n")
//             for x,l in wid,ccc
//                 write(writer,"|{l}")
//             write(writer,"|\n")
//         for x,l in wid
//             write(writer,"+{strings::repeat(\"-\",l)}")
//         write(writer,"+\n")

def is_handmade(mod : Module?) {
    // several modules are handmade even though they are built-in
    let builtins = [
        "ast_boost",
        "builtin",
        "strings_boost",
        "regex",
        "regex_boost",
        "rst"
    ]
    for (name in builtins) {
        if (mod.name == name) {
            return true
        }
    }
    return mod.moduleFlags.builtIn && !mod.moduleFlags.promoted
}

def topic(category : string; mod : Module?; name : string implicit) {
    let path = is_handmade(mod) ? "handmade/" : "detail/"
    return "|{path}{category}-{module_name(mod)}-{name}|"
}

def topic(mod : Module?) {
    let path = "handmade/"
    return "|{path}module-{module_name(mod)}|"
}

def document_topic(doc_file : file; topic, content : string implicit) {
    document_topic(doc_file, topic, content) $(txt) {
        return txt
    }
}

def document_function_topic(doc_file : file; resType : smart_ptr<TypeDecl>&; topic, content : string implicit; var args : array<array<string>>) {

    var paramsIdx = -1
    var returnsIdx = -1
    let header = "Arguments"
    document_topic(doc_file, topic, content) $(txt) {

        var addDescription = false
        var lines <- txt |> replace("\r\n", "\n") |> split("\n")
        var lineIdx = 0
        while (lineIdx < length(lines)) {
            let l = lines[lineIdx]
            var found = false
            if (l |> starts_with(":param")) {
                var parts <- l |> slice(7) |> split(":")
                var name = parts[0] |> trim()
                for (arg, idx in args, count()) {
                    continue if (idx == 0)
                    if (arg[0] == name) {
                        if (paramsIdx < 0) {
                            paramsIdx = lineIdx
                        }
                        addDescription = true
                        found = true
                        arg |> push(parts[1])
                        break
                    }
                }
                if (!found) {
                    var inscope names <- [for (arg in args); arg[0]]
                    error("function {topic} has no argument '{name}' names {names}")
                    PANIC("invalid documentation")
                }
            } elif (l |> starts_with(":returns:")) {
                lines[lineIdx] = l |> replace(":returns:", ":Returns: * {describe_type(resType)} - ")
                returnsIdx = lineIdx
            }

            if (found) {
                lines |> erase(lineIdx)
            } else {
                lineIdx ++
            }
        }

        if (addDescription) {
            args[0] |> push("description")

            for (i in range(1, length(args))) {
                if (length(args[i]) < length(args[0])) {
                    error("function {topic} doesn't have enough documentation for argument {args[i][0]}")
                    PANIC("invalid documentation")
                    while (length(args[i]) < length(args[0])) {
                        args[i] |> push("")
                    }
                }
            }
        }

        if (length(args) > 1) {
            if (paramsIdx >= 0) {
                lines |> push(make_table(header, args), paramsIdx)
            } elif (returnsIdx >= 0) {
                lines |> push(make_table(header, args), max(returnsIdx - 1, 0))
            }
        }


        return lines |> join("\n")
    }

    if (paramsIdx == -1 && returnsIdx == -1 && length(args) > 1) {
        fwrite(doc_file, make_table(header, args))
    }
}

def document_topic(header : string; doc_file : file; topic, content : string implicit; var tab : array<array<string>>) {
    document_topic(doc_file, topic, content) $(txt) {
        if (add_stub_header && txt |> starts_with(STUB)) {
            if (length(tab) > 1) {
                fwrite(doc_file, make_table(header, tab))
            }
            return txt
        }

        var hasDoc = false
        txt |> replace("\r\n", "\n") |> split("\n") $(lines) {
            var got = length(lines)
            while (got > 0 && trim(lines[got - 1]) == "") {
                got -= 1
            }
            let expected = length(tab) - 1
            if (got < expected) {
                if (strict_struct_fields) {
                    error("{topic} has less documentation than values. Expected at least {expected} lines ({[for (i in range(1, length(tab))); tab[i][0]]}), got {got}\n")
                    PANIC("invalid documentation")
                }

                for (i in range(got)) {
                    fwrite(doc_file, "{lines[i]}\n")
                }

                if (got > 0) {
                    fwrite(doc_file, "\n")
                }

                if (length(tab) > 1) {
                    fwrite(doc_file, make_table(header, tab))
                }

                hasDoc = true
                return
            }

            let headerLen = got - expected
            for (i in range(headerLen)) {
                fwrite(doc_file, "{lines[i]}\n")
            }

            if (headerLen > 0) {
                fwrite(doc_file, "\n")
            }

            for (lineIdx, idx in range(headerLen, got), count(1)) {
                tab[idx] |> push("{lines[lineIdx]}")
            }

            tab[0] |> push("description")

            if (length(tab) > 1) {
                fwrite(doc_file, make_table(header, tab))
            }
            hasDoc = true
        }

        return "" if (hasDoc)

        if (length(tab) > 1) {
            fwrite(doc_file, make_table(header, tab))
        }
        return txt
    }
}

def document_topic(doc_file : file; topic, content : string implicit; cb : block<(text : string) : string>) {
    let is_handmade_topic = topic |> starts_with("|handmade/")
    let base_root = is_handmade_topic ? handmade_root : topic_root
    let topic_file_name = "{base_root}/{slice(topic,1,-1)}.rst"
    // print("documenting topic: {topic} -> {topic_file_name}\n{content}\n")
    mkdir_rec(dir_name(topic_file_name))
    var st : FStat
    if (is_handmade_topic && !stat(topic_file_name, st)) {
        fopen(topic_file_name, "wb") $(topic_file) {
            if (topic_file != null) {
                if (add_stub_header) {
                    fwrite(topic_file, "{STUB}\n")
                }
                if (content != "") {
                    fwrite(topic_file, "{content}\n")
                }
            } else {
                PANIC("can't open {topic_file_name}")
            }
        }
    }

    if (stat(topic_file_name, st)) {
        fopen(topic_file_name, "rb") $(topic_file) {
            if (topic_file != null) {
                let instxt = cb |> invoke(fread(topic_file))
                fwrite(doc_file, instxt)
                fwrite(doc_file, "\n\n")
            } else {
                PANIC("can't open {topic_file_name}")
            }
        }
    } elif (add_empty_topic) {
        // write content directly when topic file is missing, instead of raw substitution reference
        if (content != "") {
            fwrite(doc_file, "{content}\n\n")
        }
    }
}

def document_bitfield(mod : Module?; doc_file : file; name : string; value : TypeDeclPtr) {
    var tab : array<array<string>>
    emplace(tab, array<string>("field", "bit"))
    for (an, bit in value.argNames, urange(32)) {
        var line : array<string>
        push(line, string(an))
        push(line, "{uint64(1ul << uint64(bit))}")
        emplace(tab, line)
    }

    let shortType = value.describe_type_short()
    for_each_global(mod) $(glob) {
        if (glob.flags.bitfield_constant && glob._type.describe_type_short() == shortType) {
            var line : array<string>
            var parts <- split(string(glob.name), "`")
            push(line, parts.length() > 2 ? parts[2] : parts[0])
            push(line, "{(glob.init as ExprConstBitfield).value}")
            emplace(tab, line)
        }
    }

    document_topic("Fields", doc_file, topic("typedef", mod, name), describe(value), tab)
}

def document_variant(mod : Module?; doc_file : file; name : string; value : TypeDeclPtr) {
    var tab : array<array<string>>
    emplace(tab, array<string>("field", "type"))
    for (an, at in value.argNames, value.argTypes) {
        var line : array<string>
        push(line, string(an))
        push(line, describe_type(at))
        emplace(tab, line)
    }
    document_topic("Variants", doc_file, topic("typedef", mod, name), describe(value), tab)
}

def document_typedef(doc_file : file; mod : Module?; name : string implicit; value) {
    if (value.baseType == Type.tBitfield || value.baseType == Type.tBitfield8 || value.baseType == Type.tBitfield16 || value.baseType == Type.tBitfield64) {
        fwrite(doc_file, make_label(alias_label_name(value)))
        fwrite(doc_file, make_domain("das:attribute"))
        fwrite(doc_file, "bitfield {name}")
        if (value.baseType == Type.tBitfield8) {
            fwrite(doc_file, " : uint8")
        } elif (value.baseType == Type.tBitfield16) {
            fwrite(doc_file, " : uint16")
        } elif (value.baseType == Type.tBitfield64) {
            fwrite(doc_file, " : uint64")
        }
        fwrite(doc_file, "\n\n")
        document_bitfield(mod, doc_file, name, value)
    } elif (value.baseType == Type.tVariant) {
        fwrite(doc_file, make_label(alias_label_name(value)))
        fwrite(doc_file, make_domain("das:attribute"))
        fwrite(doc_file, "variant {name}")
        fwrite(doc_file, "\n\n")
        document_variant(mod, doc_file, name, value)
    } else {
        fwrite(doc_file, make_label(alias_label_name(value)))
        fwrite(doc_file, make_domain("das:attribute"))
        fwrite(doc_file, name)
        var tp = describe(value, false, false, false)
        tp = replace(tp, "$::", "")
        fwrite(doc_file, " = {tp}\n\n")
        document_topic(doc_file, topic("typedef", mod, name), "typedef {name} = {describe(value)}")
    }
}

def document_typedefs(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_typedef(mod) $(name, value) {
            if (value.flags.isPrivateAlias) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Type aliases"))
    for (mod in mods) {
        for_each_typedef(mod) $(name, value) {
            if (value.flags.isPrivateAlias) {
                return
            }
            document_typedef(doc_file, mod, name, value)
        }
    }
}

def document_global_constant(doc_file : file; mod : Module?; value : VariablePtr) {
    fwrite(doc_file, make_label(global_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, string(value.name))
    fwrite(doc_file, " = {describe(value.init)}\n\n")
    document_topic(doc_file, topic("Variable", mod, string(value.name)), describe(value))
}

def private is_global_constant(value : VariablePtr) {
    if (value.flags.private_variable) {
        return false
    }
    if (!value._type.flags.constant) {
        return false
    }
    if (value.flags.bitfield_constant) {
        return false
    }
    // without this condition we supports more types
    // if !value._type.isWorkhorseType
    //     return false
    if (value.init == null) {
        return false
    }
    return true
}

def document_global_constants(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_global(mod) $(value) {
            if (!any && is_global_constant(value)) {
                any = true
            }
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Constants"))
    for (mod in mods) {
        for_each_global(mod) $(value) {
            if (is_global_constant(value)) {
                document_global_constant(doc_file, mod, value)
            }
        }
    }
}


def short_enum_value(value : string) : string {
    if (value.ends_with("u8")) {
        return value.slice(0, -2)
    }
    return value
}

def public document_enumeration(doc_file : file; mod : Module?; value) {
    //! Writes RST documentation for a single enumeration to the doc file.
    fwrite(doc_file, make_label(enum_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")
    var tab : array<array<string>>
    emplace(tab, array<string>("name", "value"))
    for (en in value.list) {
        var line : array<string>
        push(line, string(en.name))
        push(line, short_enum_value(describe(en.value)))
        emplace(tab, line)
    }
    document_topic("Values", doc_file, topic("enumeration", mod, string(value.name)), "enum {value.name}", tab)
}


def public document_enumerations(doc_file : file; mods : array<Module?>) : bool {
    //! Writes RST documentation for all public enumerations across the given modules.
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_enumeration(mod) $(value) {
            if (any || value.isPrivate) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return false
    }
    fwrite(doc_file, make_group("Enumerations"))
    for (mod in mods) {
        for_each_enumeration(mod) $(value) {
            if (value.isPrivate) {
                return
            }
            document_enumeration(doc_file, mod, value)
        }
    }
    return true
}

def argument_needs_documenting(tt) {
    if (tt.baseType == Type.fakeContext) {
        return false
    }
    if (tt.baseType == Type.fakeLineInfo) {
        return false
    }
    return true
}

def document_function_declaration(doc_file : file; domain, func_name : string;
                                  argNames : array<string>; argTypes : array<TypeDecl?>; argInits : array<Expression?>; resType : TypeDeclPtr; drop_args : int) {
    fwrite(doc_file, make_domain(domain))
    fwrite(doc_file, "{func_name}(")
    var drop = drop_args
    var first = true
    for (an, at, i in argNames, argTypes, count()) {
        continue if (drop-- > 0)
        if (argument_needs_documenting(at)) {
            if (!first) {
                fwrite(doc_file, "; ")
            }
            first = false
            fwrite(doc_file, "{an}: {describe_type_short(at)}")
            if (length(argInits) > i && argInits[i] != null) {
                fwrite(doc_file, " = {describe_short(argInits[i])}")
            }
        }
    }
    fwrite(doc_file, ")")
    if (resType != null && !resType.isVoid) {
        fwrite(doc_file, " : {describe_type_short(resType)}")
    }
    fwrite(doc_file, "\n\n")
}

def document_function_arguments(doc_file : file; argNames : array<string>; argTypes : array<TypeDecl?>; skip_args : int) : array<array<string>> {
    var tab : array<array<string>>
    emplace(tab, array<string>("argument", "type"))
    var skip = skip_args
    for (an, at in argNames, argTypes) {
        continue if (skip-- > 0)
        if (argument_needs_documenting(at)) {
            var line : array<string>
            push(line, an)
            push(line, describe_type(at))
            emplace(tab, line)
        }
    }
    return <- tab
}

def extract_func(var expr : ExpressionPtr) : ast::Function? {
    if (expr == null) {
        return null
    }
    if (expr is ExprAddr) {
        return (expr as ExprAddr).func
    } elif (expr is ExprCast) {
        return extract_func((expr as ExprCast).subexpr)
    }
    return null
}

def document_class_method(doc_file : file; mod : Module?; value; var fld) {
    var func = extract_func(fld.init)
    if (func == null) {
        return
    }

    fwrite(doc_file, make_label(function_label_name(func)))

    let argNames <- [for (arg in func.arguments); string(arg.name)]
    let argTypes <- [for (arg in func.arguments); get_ptr(arg._type)]
    let argInits <- [for (arg in func.arguments); get_ptr(arg.init)]
    (document_function_declaration(doc_file, "das:function", "{value.name}.{fld.name}",
        argNames, argTypes, argInits, fld._type.firstType, /*drop args*/1))
    // if fld._type.firstType != null && !fld._type.firstType.isVoid
    //     fwrite(doc_file,"``{fld.name}`` returns {describe_type(fld._type.firstType)}\n\n")
    var args <- document_function_arguments(doc_file, argNames, argTypes, /*skip args*/1)
    document_function_topic(doc_file, func.result, topic("function", mod, "{value.name}-{function_label_file(fld.name, fld.init._type, 1)}"), "def {fld.name} : {describe(fld._type)}", args)
}

def document_struct_method(doc_file : file; mod : Module?; value; var func) {
    // For structs (not classes), methods become module-level functions with isClassMethod flag.
    // Static methods get names like "BoolArray`clear"; non-static keep names like "[]", "finalize".
    // We use moreFlags.isStaticClassMethod to distinguish them and rebuild the detail file key.
    var fname = string(func.name)
    let is_static = func.moreFlags.isStaticClassMethod
    if (is_static) {
        fname = fname.slice(length(value.name) + 1)
    }

    fwrite(doc_file, make_label(function_label_name(func)))

    let argNames <- [for (arg in func.arguments); string(arg.name)]
    let argTypes <- [for (arg in func.arguments); get_ptr(arg._type)]
    let argInits <- [for (arg in func.arguments); get_ptr(arg.init)]
    (document_function_declaration(doc_file, "das:function", "{value.name}.{fname}",
        argNames, argTypes, argInits, func.result, /*drop args*/1))

    if (func.flags.unsafeOperation) {
        document_warning(doc_file, "This is unsafe operation.")
    }
    if (func.moreFlags.deprecated) {
        document_warning(doc_file, "This function is deprecated.")
    }

    var args <- document_function_arguments(doc_file, argNames, argTypes, /*skip args*/1)

    // Build topic key matching rst_comment.das pre-inference output.
    // Pre-inference: method name is unqualified, and:
    //   non-static methods (isStaticClassMethod=false): self NOT in args (added during inference) → skip 1 for hash
    //   static methods (isStaticClassMethod=true): self IS in args (explicit) → skip 0 for hash
    let hash_drop = is_static ? 0 : 1
    var mn = build_string() <| $(var w) {
        w |> write(fname)
        var drop = hash_drop
        for (arg in func.arguments) {
            if (drop-- > 0) {
                continue
            }
            let prefix = arg._type.flags.removeConstant ? "var " : ""
            w |> write("{prefix}{describe_type_short(arg._type)}")
        }
    }
    let file_label = "{safe_function_name(fname)}-{hash(mn)}"
    document_function_topic(doc_file, func.result, topic("function", mod, "{value.name}-{file_label}"), "def {value.name}.{fname}", args)
}

def make_requirements_list(value : smart_ptr<ast::Structure>) {
    var inscope requiredComps : array<string>
    for (ann in value.annotations) {
        if (ann.annotation.name == "require") {
            for (arg in ann.arguments) {
                requiredComps |> push("``{arg.sValue}``")
            }
        }
    }
    var compsCount = length(requiredComps)
    if (compsCount == 0) {
        return ""
    }
    return ".. note:: {join(requiredComps) <| ","}  component{compsCount > 1 ? "s": ""} {compsCount > 1 ? "are" : "is"} automatically added to the node if {compsCount > 1 ? "they are": "it is"} not already present\n\n"
}

def is_trivial_ctor(func : FunctionPtr) {
    if (func.body == null) {
        return true
    }

    if (func.body is ExprBlock) {
        var blk = func.body as ExprBlock
        if (length(blk.list) == 1) {
            return blk.list[0] is ExprReturn
        }
    }

    return false
}

def is_class_method(value; fld : FieldDeclaration&) {
    return fld.flags.classMethod
}
    // return length(fld._type.argNames) > 0 && fld._type.argNames[0] == "self" && is_class_method(unsafe(reinterpret<StructurePtr> value), fld._type)

def document_classes(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_structure(mod) $(value) {
            if (!any && value.flags.isClass && !value.flags.privateStructure) {
                any = true
            }
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Classes"))
    for (mod in mods) {
        for_each_structure(mod) $(value) {
            if (!value.flags.isClass || value.flags.privateStructure) {
                return
            }
            fwrite(doc_file, make_label(struct_label_name(value)))
            fwrite(doc_file, make_domain("das:attribute"))
            fwrite(doc_file, string(value.name))
            if (value.parent != null) {
                fwrite(doc_file, " : {value.parent.name}")
            }
            fwrite(doc_file, "\n\n")
            fwrite(doc_file, make_requirements_list(value))

            var fields : array<array<string>>
            fields |> emplace(array<string>("field", "type"))
            var hasValues = false
            for (fld in value.fields) {
                if (!is_class_method(value, fld) && !(fld.flags.generated) && fld.flags.implemented && !fld.flags.privateField && !(fld.flags.parentType) && (fld.name != "__rtti")) {
                    if (fld.init != null) {
                        hasValues = true
                        fields[0] |> push("value")
                        break
                    }
                }
            }

            let staticFieldPrefix = "{value.name}`"
            for_each_global(mod) $(gVar) {
                if (!gVar.flags.private_variable && string(gVar.name).starts_with(staticFieldPrefix)) {
                    var line : array<string>
                    push(line, "{value.name}.{string(gVar.name).slice(length(staticFieldPrefix))}")
                    push(line, describe_type(gVar._type))
                    if (hasValues) {
                        push(line, gVar.init != null ? describe(gVar.init) : "")
                    }
                    emplace(fields, line)
                }
            }

            for (fld in value.fields) {
                if (!is_class_method(value, fld) && !(fld.flags.generated) && fld.flags.implemented && !fld.flags.privateField && !(fld.flags.parentType) && (fld.name != "__rtti")) {
                    var line : array<string>
                    push(line, string(fld.name))
                    push(line, describe_type(fld._type))
                    if (hasValues) {
                        push(line, fld.init != null ? describe(fld.init) : "")
                    }
                    emplace(fields, line)
                }
            }

            document_topic("Fields", doc_file, topic("class", mod, string(value.name)), "class {value.name}", fields)

            for (fld in value.fields) {
                if (is_class_method(value, fld) && !(fld.flags.generated) && !fld.flags.privateField && !(fld.flags.parentType) && (fld.flags.implemented)) {
                    document_class_method(doc_file, mod, value, fld)
                }
            }
            var first = true
            var inscope annT <- new TypeDecl(baseType = Type.tStructure, structType = get_ptr(value))
            for_each_function(mod, "") $(func) {
                if (func.flags.isClassMethod && func.classParent == value && func.name == value.name && !is_trivial_ctor(func)) {
                    // ctor
                    document_function(doc_file, mod, func, /*drop first arg*/0, "{value.name}-", "das:function")
                    return
                }
                if (!(func.moreFlags.propertyFunction || func.flags.isClassMethod) || func.arguments |> length == 0) {
                    return
                }
                let fna = string(func.name)
                if (!fna |> starts_with(".`") || length(fna) <= 2) {
                    return
                }
                if (!is_same_type(func.arguments[0]._type, annT, RefMatters.no, ConstMatters.no, TemporaryMatters.no)) {
                    return
                }
                if (first) {
                    fwrite(doc_file, ":Properties:\n\n")
                    first = false
                }
                document_function(doc_file, mod, func, /*drop first arg*/1, "{value.name}-", "das:operator")
            }
        }
    }
}

def document_typemacro(doc_file : file; mod : Module?; value) {
    var found = false
    for (ann in value.annotations) {
        if (ann.annotation.name == "typemacro_documentation") {
            found = true
            // first arg is name, the rest are pairs argument:value
            var first = true
            var tab : array<array<string>>
            for (arg in ann.arguments) {
                if (first) {
                    fwrite(doc_file, make_label(call_macro_label_name(mod, string(arg.name))))
                    fwrite(doc_file, make_domain("das:attribute"))
                    fwrite(doc_file, "type macro {arg.name}")
                    fwrite(doc_file, "\n\n")
                    first = false
                } else {
                    if (arg.basicType == Type.tBool) {
                        tab.emplace([string(arg.name), "Bool = {arg.bValue}"])
                    } elif (arg.basicType == Type.tString) {
                        if (arg.sValue == "```empty```type```") {
                            tab.emplace([string(arg.name), "Type"])
                        } else {
                            tab.emplace([string(arg.name), "String = \"{arg.sValue}\""])
                        }
                    } elif (arg.basicType == Type.tInt) {
                        tab.emplace([string(arg.name), "Integer = {arg.iValue}"])
                    } else {
                        tab.emplace([string(arg.name), "Type"])
                    }
                }
            }
            fwrite(doc_file, make_table("Arguments", tab))
        }
    }
    if (found) return
    fwrite(doc_file, "{value}\n\n")

}

def document_typemacros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_structure(mod) $(value) {
            if (any || value.parent == null ||  value.parent.name != "AstTypeMacro" || value.parent._module.name != "ast_core") {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Type macros"))
    for (mod in mods) {
        for_each_structure(mod) $(value) {
            if (value.parent == null ||  value.parent.name != "AstTypeMacro" || value.parent._module.name != "ast_core") {
                return
            }
            document_typemacro(doc_file, mod, value)
        }
    }
}

def document_structure(doc_file : file; mod : Module?; var value : StructurePtr) {
    fwrite(doc_file, make_label(struct_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}")
    if (value.parent != null) {
        fwrite(doc_file, " : {value.parent.name}")
    }
    fwrite(doc_file, "\n\n")
    var tab : array<array<string>>
    emplace(tab, array<string>("field", "type"))
    var hasValues = false
    for (fld in value.fields) {
        if (!is_class_method(value, fld) && !(fld.flags.generated) && fld.flags.implemented && !fld.flags.privateField && !(fld.flags.parentType) && (fld.name != "__rtti")) {
            if (fld.init != null) {
                hasValues = true
                tab[0] |> push("value")
                break
            }
        }
    }
    for (fld in value.fields) {
        if (!is_class_method(value, fld) && !(fld.flags.generated) && fld.flags.implemented && !fld.flags.privateField && !(fld.flags.parentType) && (fld.name != "__rtti")) {
            var line : array<string>
            push(line, string(fld.name))
            push(line, describe_type(fld._type))
            if (hasValues) {
                push(line, fld.init != null ? describe(fld.init) : "")
            }
            emplace(tab, line)
        }
    }
    // fwrite(doc_file,"{value.name} fields are\n\n")
    // fwrite(doc_file,make_table(tab))
    document_topic("Fields", doc_file, topic("structure", mod, string(value.name)), "struct {value.name}", tab)

    for (fld in value.fields) {
        if (is_class_method(value, fld) && !fld.flags.generated && !fld.flags.privateField && !fld.flags.parentType && fld.flags.implemented) {
            document_class_method(doc_file, mod, value, fld)
        }
    }

    // Document struct methods (module-level functions with isClassMethod flag).
    // For structs (unlike classes), methods are not stored as struct fields after inference.
    for_each_function(mod, "") $(func) {
        if (!func.flags.isClassMethod || func.classParent != value) {
            return
        }
        if (func.flags.generated) {
            return
        }
        // Skip constructors (handled below)
        if (func.name == value.name) {
            return
        }
        // Skip property functions (handled below)
        if (string(func.name) |> starts_with(".`")) {
            return
        }
        if (!func.moreFlags.isStaticClassMethod) {
            if (func.name == "{value.name}`{value.name}") {
                // Foo`Foo == generated ctor
                return
            }
            for (fld in value.fields) {
                // exclude instantiated struct fields
                if (fld.flags.classMethod && fld.flags.implemented && func.name == "{value.name}`{fld.name}") {
                    return
                }
            }
        }
        document_struct_method(doc_file, mod, value, func)
    }

    var inscope annT <- new TypeDecl(baseType = Type.tStructure, structType = get_ptr(value))
    var first = true
    for_each_function(mod, "") $(func) {
        if (func.flags.isClassMethod && func.classParent == value && func.name == value.name && !is_trivial_ctor(func)) {// ctor
            document_function(doc_file, mod, func, /*drop first arg*/0, "{value.name}-", "das:function")
            return
        }
        if (!(func.moreFlags.propertyFunction || func.flags.isClassMethod) || func.arguments |> length == 0) {
            return
        }
        let fna = string(func.name)
        if (!fna |> starts_with(".`") || length(fna) <= 2) {
            return
        }
        if (!is_same_type(func.arguments[0]._type, annT, RefMatters.no, ConstMatters.no, TemporaryMatters.no)) {
            return
        }
        if (first) {
            fwrite(doc_file, ":Properties:\n\n")
            first = false
        }
        document_function(doc_file, mod, func, /*drop first arg*/1, "{value.name}-", "das:operator")
    }
}

def document_structures(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_structure(mod) $(value) {
            if (any || value.flags.isClass || value.flags.generated || value.flags.privateStructure) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Structures"))
    for (mod in mods) {
        for_each_structure(mod) $(value) {
            if (value.flags.isClass || value.flags.generated || value.flags.privateStructure) {
                return
            }
            document_structure(doc_file, mod, value)
        }
    }
}

def document_structure_annotation(doc_file : file; mod : Module?; value) {
    fwrite(doc_file, make_label(handle_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")

    var props : array<array<string>>
    props |> emplace(array<string>("field", "type"))
    var inscope annT <- new TypeDecl(baseType = Type.tHandle,
        annotation = unsafe(reinterpret<TypeAnnotation?> value))
    for_each_function(mod, "") $(func) {
        if (!func.moreFlags.propertyFunction) {
            return
        }
        if (func.arguments |> length != 1) {
            return
        }
        let fna = string(func.name)
        if (!fna |> starts_with(".`") || length(fna) <= 2) {
            return
        }
        if (!is_same_type(func.arguments[0]._type, annT, RefMatters.no, ConstMatters.no, TemporaryMatters.no)) {
            return
        }
        var line : array<string>
        push(line, slice(fna, 2))
        push(line, describe_type(func.result))
        emplace(props, line)
        document_function(doc_file, mod, func, /*drop first arg*/1)
    }
    if (length(props) > 1) {
        // fwrite(doc_file,"{value.name} property operators are\n\n")
        fwrite(doc_file, make_table("Properties", props))
    }

    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)

    var tab : array<array<string>>
    tab |> emplace(array<string>("field", "type"))
    var fields : array<tuple<name : string; typ : string; offset : uint>>
    for_each_field(*ann) $(name, cppName, xtype, offset) {
        fields |> emplace((name, describe_type(xtype), offset))
    }

    fields |> sort($(a, b) => a.offset < b.offset)
    tab |> reserve(length(fields))
    for (f in fields) {
        var line : array<string>
        push(line, f.name)
        push(line, f.typ)
        emplace(tab, line)
    }

    document_topic("Fields", doc_file, topic("structure_annotation", mod, string(value.name)), "Annotation {value.name}", tab)
}

def document_structure_annotations(doc_file : file; mods : array<Module?>; var hook = DocsHook()) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        module_for_each_annotation(mod) $(value) {
            if (any || !value.isBasicStructureAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Handled structures"))
    for (mod in mods) {
        module_for_each_annotation(mod) $(value) {
            if (!value.isBasicStructureAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            document_structure_annotation(doc_file, mod, value)
        }
    }
}

def document_enumeration_annotation(doc_file : file; mod : Module?; value) {
    fwrite(doc_file, make_label(handle_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")
    document_topic(doc_file, topic("enumeration_macro", mod, string(value.name)), "Enumeration annotation {value.name}")
}

def document_enumeration_annotations(doc_file : file; mods : array<Module?>; var hook = DocsHook()) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        module_for_each_annotation(mod) $(value) {
            if (!value.isEnumerationAnnotation) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Handled enumerations"))
    for (mod in mods) {
        module_for_each_annotation(mod) $(value) {
            if (!value.isEnumerationAnnotation) {
                return
            }
            document_enumeration_annotation(doc_file, mod, value)
        }
    }
}


def document_function_annotation(doc_file : file; mod : Module?; value) {
    fwrite(doc_file, make_label(handle_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")
    document_topic(doc_file, topic("function_annotation", mod, string(value.name)), "Function annotation {value.name}")
}

def document_function_annotations(doc_file : file; mods : array<Module?>; var hook = DocsHook()) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        module_for_each_annotation(mod) $(value) {
            if (any || !value.isFunctionAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Function annotations"))
    for (mod in mods) {
        module_for_each_annotation(mod) $(value) {
            if (!value.isFunctionAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            document_function_annotation(doc_file, mod, value)
        }
    }
}

def document_annotation(doc_file : file; mod : Module?; value) {
    fwrite(doc_file, make_label(handle_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")
    document_topic(doc_file, topic("annotation", mod, string(value.name)), "Annotation {value.name}")
}

def document_annotations(doc_file : file; mods : array<Module?>; var hook = DocsHook()) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        module_for_each_annotation(mod) $(value) {
            if (any || !value.isTypeAnnotation || value.isEnumerationAnnotation || value.isBasicStructureAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Handled types"))
    for (mod in mods) {
        module_for_each_annotation(mod) $(value) {
            if (!value.isTypeAnnotation || value.isEnumerationAnnotation || value.isBasicStructureAnnotation || (hook.annotationFilter != null && !hook.annotationFilter |> invoke(value))) {
                return
            }
            if (string(value.name) |> starts_with("dasvector`")) {
                return
            }
            document_annotation(doc_file, mod, value)
        }
    }
}

def document_structure_macro(doc_file : file; mod : Module?; value) {
    fwrite(doc_file, make_label(handle_label_name(value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value.name}\n\n")
    document_topic(doc_file, topic("structure_macro", mod, string(value.name)), "Structure annotation {value.name}")
}

def document_structure_macros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        module_for_each_annotation(mod) $(value) {
            if (any || value.isTypeAnnotation || value.isEnumerationAnnotation || value.isBasicStructureAnnotation || value.isFunctionAnnotation) {
                return
            }
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Structure macros"))
    for (mod in mods) {
        module_for_each_annotation(mod) $(value) {
            if (value.isTypeAnnotation || value.isEnumerationAnnotation || value.isBasicStructureAnnotation || value.isFunctionAnnotation) {
                return
            }
            document_structure_macro(doc_file, mod, value)
        }
    }
}

def document_call_macro(doc_file : file; mod : Module?; value : string implicit) {
    fwrite(doc_file, make_label(call_macro_label_name(mod, value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value}\n\n")
    document_topic(doc_file, topic("function_annotation", mod, value), "Function annotation {value}")
}

def document_call_macros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_call_macro(mod) $(value) {
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Call macros"))
    for (mod in mods) {
        for_each_call_macro(mod) $(value) {
            document_call_macro(doc_file, mod, value)
        }
    }
}

def document_reader_macro(doc_file : file; mod : Module?; value : string implicit) {
    fwrite(doc_file, make_label(call_macro_label_name(mod, value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value}\n\n")
    document_topic(doc_file, topic("reader_macro", mod, value), "Reader macro {value}")
}

def document_reader_macros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_reader_macro(mod) $(value) {
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Reader macros"))
    for (mod in mods) {
        for_each_reader_macro(mod) $(value) {
            document_reader_macro(doc_file, mod, value)
        }
    }
}

def document_variant_macro(doc_file : file; mod : Module?; value : string implicit) {
    fwrite(doc_file, make_label(call_macro_label_name(mod, value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value}\n\n")
    document_topic(doc_file, topic("variant_macro", mod, value), "Variant macro {value}")
}

def document_variant_macros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_variant_macro(mod) $(value) {
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Variant macros"))
    for (mod in mods) {
        for_each_variant_macro(mod) $(value) {
            document_variant_macro(doc_file, mod, string(value.name))
        }
    }
}

def document_typeinfo_macro(doc_file : file; mod : Module?; value : string implicit) {
    fwrite(doc_file, make_label(call_macro_label_name(mod, value)))
    fwrite(doc_file, make_domain("das:attribute"))
    fwrite(doc_file, "{value}\n\n")
    document_topic(doc_file, topic("typeinfo_macro", mod, value), "Typeinfo macro {value}")
}

def document_typeinfo_macros(doc_file : file; mods : array<Module?>) {
    var any = false
    for (mod in mods) {
        if (any) {
            break
        }
        for_each_typeinfo_macro(mod) $(value) {
            any = true
        }
    }
    if (!any) {
        return
    }
    fwrite(doc_file, make_group("Typeinfo macros"))
    for (mod in mods) {
        for_each_typeinfo_macro(mod) $(value) {
            document_typeinfo_macro(doc_file, mod, string(value.name))
        }
    }
}

def function_needs_documenting(func : FunctionPtr | Function?) {
    if (func.flags.generated) {
        return false
    }
    if (func.flags.isClassMethod) {
        return false
    }
    if (func.flags.privateFunction) {
        return false
    }
    if (func.fromGeneric != null) {
        return false
    }
    if (func.moreFlags.propertyFunction) {
        return false
    }
    var isBuiltin = false
    peek(func._module.name) $(name) {
        if (name == "$" || name == "builtin") {
            isBuiltin = true
        }
    }
    // skip builtin operators
    var isOperator = false
    if (isBuiltin) {
        peek(func.name) $(name) {
            let ch = first_character(name)
            if (!is_alpha(ch) && !is_number(ch) && ch != '_') {
                isOperator = true
            }
        }
    }
    if (isOperator) {
        return false
    }
    // skip builtin type c'tors
    var isTypeConstructor = false
    if (isBuiltin) {
        peek(func.name) $(name) {
            if (func.result != null && func.result.isCtorType) {
                let tname = das_to_string(func.result.baseType)
                if (name == tname) {
                    isTypeConstructor = true
                }
            }
        }
    }
    if (isTypeConstructor) {
        return false
    }
    // skip ones starting with _builtin or __builtin
    var isBuiltinName = false
    peek(func.name) $(name) {
        if (starts_with(name, "_builtin") || starts_with(name, "__builtin")) {
            isBuiltinName = true
        }
    }
    if (isBuiltinName) {
        return false
    }
    return true
}

def document_warning(doc_file : file; issue : string) {
    fwrite(doc_file, ".. warning::")
    fwrite(doc_file, "\n  {issue}\n\n")
}

def function_name(func : FunctionPtr | Function?) {
    let name = "{func.name}"
    if (!empty(func.name)) {
        let ch = first_character(name)
        if (ch != '_' && !is_alpha(ch)) {
            var safeName = name
            // `is`NAME / `as`NAME → "operator is NAME" / "operator as NAME"
            if (safeName |> starts_with("`is`")) { return "operator is {safeName |> slice(4)}"; }
            if (safeName |> starts_with("`as`")) { return "operator as {safeName |> slice(4)}"; }
            // bare `is / `as → "operator is" / "operator as"
            if (safeName == "`is") { return "operator is"; }
            if (safeName == "`as") { return "operator as"; }
            // ?as`NAME → "operator ?as NAME"; bare ?as → "operator ?as"
            if (safeName |> starts_with("?as`")) { return "operator ?as {safeName |> slice(4)}"; }
            if (safeName == "?as") { return "operator ?as"; }
            // ?.`NAME → Type?.name (safe navigation property)
            if (safeName |> starts_with("?.`")) { safeName = "?.{safeName |> slice(3)}"; }
            // .`NAME → Type.name (property accessor)
            elif (safeName |> starts_with(".`")) { safeName = ".{safeName |> slice(2)}"; }
            // .name`clone → ".name :="  (clone-assign on property)
            if (safeName |> ends_with("`clone")) {
                safeName = "{safeName |> slice(0,-6)} :="
            }
            // .name`&&= / `||= / `^^= → ".name &&=" etc (double-char compound assign)
            if (length(safeName) > 4 && safeName |> ends_with("=")) {
                let p4 = safeName.character_at(length(safeName) - 4) // nolint:PERF003 single access for operator parsing
                if (p4 == '`') {
                    let op2 = safeName |> slice(length(safeName) - 3, length(safeName))
                    safeName = "{safeName |> slice(0, length(safeName) - 4)} {op2}"
                }
            }
            // .name`+= / `-= / `*= etc → ".name +=" (single-char compound assign)
            if (length(safeName) > 3 && safeName |> ends_with("=") && safeName.character_at(length(safeName) - 3) == '`') { // nolint:PERF003
                let op = character_at(safeName, length(safeName) - 2) // nolint:PERF003
                safeName = "{safeName |> slice(0,-3)} {op.to_char()}="
            }
            return (length(func.arguments) == 0
                ? safeName
                : "{func.arguments[0]._type |> describe_type_short()}{safeName}"
            )
        }
    }
    return name
}

def document_function(doc_file : file; mod : Module?; var func : FunctionPtr | Function?; drop_args : int = 0; prefix : string = ""; objType : string = "das:function"; skip_body : bool = false) {
    let argNames <- [for (arg in func.arguments); string(arg.name)]
    let argTypes <- [for (arg in func.arguments); get_ptr(arg._type)]
    let argInits <- [for (arg in func.arguments); get_ptr(arg.init)]
    fwrite(doc_file, make_label(function_label_name(func)))
    let funcSafeName = function_name(func)
    (document_function_declaration(doc_file, objType, funcSafeName,
        argNames, argTypes, argInits, func.result, drop_args))
    if (skip_body) {
        return
    }
    // if func.result != null && !func.result.isVoid
    //     fwrite(doc_file,"``{funcSafeName}`` returns {describe_type(func.result)}\n\n")
    if (func.flags.unsafeOperation) {
        document_warning(doc_file, "This is unsafe operation.")
    }
    if (func.moreFlags.deprecated) {
        document_warning(doc_file, "This function is deprecated.")
    }

    var args <- document_function_arguments(doc_file, argNames, argTypes, drop_args)
    document_function_topic(doc_file, func.result, topic("function", mod, "{prefix}{function_label_file(func,drop_args)}"), "def {rst_describe_function_short(func)}", args)
}


def document_functions(doc_file : file; mods : array<Module?>; var groups : array<DocGroup>) {
    var tab : table<Function?>
    for (grp in groups) {
        if (length(grp.func) != 0) {
            if (!show_hidden_groups && grp.hidden) {
                for (func in grp.func) {
                    tab.insert(func.fn)
                }
            } else {
                fwrite(doc_file, make_group(grp.name))
                if (grp.hidden) {
                    document_warning(doc_file, "This group of functions is hidden. It will not be in the final documentation.")
                }
                grp.func |> sort($(a, b) => function_name(a.fn) < function_name(b.fn))
                for (func in grp.func) {
                    if (!key_exists(tab, func.fn)) {
                        let descr = rst_describe_function_short(func.fn)
                        var labn = function_label_name(func.fn)
                        fwrite(doc_file, "  * {make_ref(labn,descr)}\n")
                    }
                }
                fwrite(doc_file, "\n")
                if (group_function_overloads) {
                    // build set of names that have multiple overloads
                    var multi_overload : table<string>
                    var prev_count_name = ""
                    var count_run = 0
                    for (func in grp.func) {
                        if (!tab.key_exists(func.fn)) {
                            let cur_name = function_name(func.fn)
                            if (cur_name == prev_count_name) {
                                count_run += 1
                                if (count_run == 2) {
                                    multi_overload |> insert(cur_name)
                                }
                            } else {
                                prev_count_name = cur_name
                                count_run = 1
                            }
                        }
                    }
                    var prev_func_name = ""
                    var prev_was_overload_group = false
                    for (func in grp.func) {
                        if (tab.key_exists(func.fn)) {
                            continue
                        }
                        tab.insert(func.fn)
                        let cur_name = function_name(func.fn)
                        let is_overload = cur_name == prev_func_name
                        if (!is_overload) {
                            if (prev_was_overload_group) {
                                fwrite(doc_file, "----\n\n")
                            }
                            if (multi_overload |> key_exists(cur_name)) {
                                fwrite(doc_file, "\n{cur_name}\n{strings::repeat("^", length(cur_name))}\n\n")
                                prev_was_overload_group = true
                            } else {
                                prev_was_overload_group = false
                            }
                        }
                        document_function(doc_file, func.mod, func.fn, 0, "", "das:function", is_overload)
                        prev_func_name = cur_name
                    }
                } else {
                    // !group_function_overloads
                    for (func in grp.func) {
                        if (tab.key_exists(func.fn)) {
                            continue
                        }
                        tab.insert(func.fn)
                        document_function(doc_file, func.mod, func.fn)
                    }
                }
            }
        }
    }
    var first_function = true
    for (mod in mods) {
        for_each_function(mod, "") $(func) {
            if (!function_needs_documenting(func)) {
                return
            }
            var pfunc = get_ptr(func)
            if (tab.key_exists(pfunc)) {
                return
            }
            tab.insert(pfunc)
            if (first_function) {
                fwrite(doc_file, make_group("Uncategorized"))
                first_function = false
            }
            document_function(doc_file, mod, func)
        }
        for_each_generic(mod) $(func) {
            if (!function_needs_documenting(func)) {
                return
            }
            var pfunc = get_ptr(func)
            if (tab.key_exists(pfunc)) {
                return
            }
            tab.insert(pfunc)
            if (first_function) {
                fwrite(doc_file, make_group("Uncategorized"))
                first_function = false
            }
            document_function(doc_file, mod, func)
        }
    }
}

struct public DocGroup {
    //! Group of functions with shared category.
    name : string
    func : array<tuple<fn : Function?; mod : Module?>>
    hidden : bool
    @do_not_delete _module : Module?
}

def debug_group(var group : DocGroup) {
    print("Group {group.name}\n")
    for (func in group.func) {
        print("\t{rst_describe_function_short(func.fn)}\n")
    }
    return <- group
}

def public append_to_group_by_regex(var group : DocGroup; var mod : Module?; var reg : Regex) : DocGroup& {
    //! Appends functions whose names match the given regex to an existing DocGroup.
    for_each_function(mod, "") $(func) {
        if (function_needs_documenting(func)) {
            if (regex_match(reg, string(func.name)) != -1) {
                push(group.func, (get_ptr(func), mod))
            }
        }
    }
    for_each_generic(mod) $(func) {
        if (function_needs_documenting(func)) {
            if (regex_match(reg, string(func.name)) != -1) {
                push(group.func, (get_ptr(func), mod))
            }
        }
    }
    unsafe {
        return group
    }
}

def public group_by_regex(name : string; var mod : Module?; var reg : Regex) {
    //! Creates a group of functions with shared category.
    //! Functions will be added to the group if they match the regular expression.
    var group <- DocGroup(name = name, hidden = false)
    append_to_group_by_regex(group, mod, reg)
    return <- group
}

def public group_by_regex(name : string; var mods : array<Module?>; var reg : Regex) {
    //! Creates a group of functions with shared category.
    //! Functions will be added to the group if they match the regular expression.
    var group <- DocGroup(name = name, hidden = false)
    for (mod in mods) {
        append_to_group_by_regex(group, mod, reg)
    }
    return <- group
}

def public hide_group(var group : DocGroup) {
    //! Marks the group as hidden.
    group.hidden = true
    return <- group
}

def private generate_topic_stub(var tab : table<string>; doc_file : file; topic : string implicit) {
    if (!tab.key_exists(topic)) {
        tab.insert(topic)
        fwrite(doc_file, ".. {topic} replace:: to be documented in {topic}.rst\n\n")
    }
}

def private generate_substitute_stub(mod : Module?; substname : string) {
    var st : FStat
    // if stat(substname,st)   // this is actually 'see if it exists'
    //     return
    mkdir_rec(dir_name(substname))
    fopen(substname, "wb") $(stub_file) {
        if (stub_file != null) {
            var tab : table<string>
            // typedefs
            for_each_typedef(mod) $(name, value) {
                generate_topic_stub(tab, stub_file, topic("typedef", mod, name))
            }
            // enumerations
            for_each_enumeration(mod) $(value) {
                generate_topic_stub(tab, stub_file, topic("enumeration", mod, string(value.name)))
            }
            // structure everything
            for_each_structure(mod) $(value) {
                if (value.flags.isClass) {
                    // classes and methods
                    generate_topic_stub(tab, stub_file, topic("class", mod, string(value.name)))
                    for (fld in value.fields) {
                        if (is_class_method(value, fld) && !(fld.flags.generated) && !(fld.flags.parentType) && (fld.flags.implemented)) {
                            generate_topic_stub(tab, stub_file, topic("method", mod, "{value.name}.{fld.name}"))
                        }
                    }
                } elif (!value.flags.generated) {
                    // structures
                    generate_topic_stub(tab, stub_file, topic("structure", mod, string(value.name)))
                }
            }
            for_each_function(mod, "") $(func) {
                if (function_needs_documenting(func)) {
                    generate_topic_stub(tab, stub_file, topic("function", mod, "{func.name}"))
                }
            }
            for_each_generic(mod) $(func) {
                if (function_needs_documenting(func)) {
                    generate_topic_stub(tab, stub_file, topic("function", mod, "{func.name}"))
                }
            }
            // annotations
            module_for_each_annotation(mod) $(value) {
                if (value.isBasicStructureAnnotation) {
                    generate_topic_stub(tab, stub_file, topic("structure_annotation", mod, string(value.name)))
                } elif (value.isTypeAnnotation) {
                    generate_topic_stub(tab, stub_file, topic("any_annotation", mod, string(value.name)))
                }
            }
            // variables
            for_each_global(mod) $(value) {
                if (is_global_constant(value)) {
                    generate_topic_stub(tab, stub_file, topic("variable", mod, string(value.name)))
                }
            }
        } else {
            PANIC("can't generate stub at {substname}\n")
        }
    }
}


def public document(name : string; var mod : Module?; fname : string; var groups : array<DocGroup>; var hook : DocsHook = DocsHook()) {
    //! Document single module given list of `DocGropus`.
    //! This will generate RST file with documentation for the module.
    //! Functions which do not match any `DocGroup` will be placed in the `Uncategorized` group.
    var mods : array<Module?> = [mod]
    documents(name, mods, fname, groups, hook)
}

struct public DocsHook {
    //! Callback hooks for customizing RST documentation output.
    annotationFilter : lambda<(ann : rtti::Annotation const) : bool>
    afterEnums : lambda<(var f : FILE const?; was_enums : bool) : void>
}


def public documents(name : string; mods : array<Module?>; fname : string; var groups : array<DocGroup>; var hook : DocsHook = DocsHook()) {
    //! Generates complete RST documentation for a set of modules into the specified file.
    static_if (log_documentation) {
        print("Documenting {name} into {fname}\n")
    }
    if (length(mods) == 0) {
        PANIC("missing module {name}")
    }
    let path = "{topic_root}/{fname}"
    mkdir_rec(dir_name(path))
    var doc_file = fopen(path, "wb")
    if (doc_file == null) {
        PANIC("can't open {path}")
    }
    fwrite(doc_file, "\n")
    var mod_name = string(mods[0].name)
    for ((src, dst) in module_name_replacements) {
        if (mod_name == src) {
            mod_name = dst
            break
        }
    }
    fwrite(doc_file, make_header(name, mod_name))
    fwrite(doc_file, ".. das:module:: {mod_name}\n\n")
    // if length(header)!=0
    //     fwrite(doc_file,".. include:: {header}\n\n")
    for (mod in mods) {
        generate_substitute_stub(mod, "{topic_root}/detail/{fname}")
    }
    for (mod in mods) {
        document_topic(doc_file, topic(mod), "Module {mod.name}")// use only first module for the topic
        break
    }

    document_typedefs(doc_file, mods)
    document_global_constants(doc_file, mods)
    let wasEnums = document_enumerations(doc_file, mods)
    if (hook.afterEnums != null) {
        hook.afterEnums |> invoke(doc_file, wasEnums)
    }
    document_structures(doc_file, mods)
    document_structure_annotations(doc_file, mods, hook)
    document_function_annotations(doc_file, mods, hook)
    document_call_macros(doc_file, mods)
    document_reader_macros(doc_file, mods)
    document_variant_macros(doc_file, mods)
    document_typeinfo_macros(doc_file, mods)
    document_annotations(doc_file, mods, hook)
    document_enumeration_annotations(doc_file, mods, hook)
    document_structure_macros(doc_file, mods)
    document_typemacros(doc_file, mods)
    document_classes(doc_file, mods)
    document_functions(doc_file, mods, groups)
    fwrite(doc_file, "\n")
    fclose(doc_file)
}
