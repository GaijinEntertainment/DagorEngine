options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module rst_comment private

require daslib/ast_boost
require fio
require strings
require rst

enum ParserState {
    none
    structure
    field
    func
    method
    global
    variant_alias
    variant_entry
    tuple_alias
    tuple_entry
    bitfield_alias
    bitfield_entry
    enum_decl
    enum_entry
    type_alias
}

let private DEBUG_COMMENT_READER = true

[macro_function]
def private debug_rst(s : string) {
    if (DEBUG_COMMENT_READER) {
        to_log(LOG_INFO, "{s}\n")
    }
}

def panic_rst(s : string) {
    if (DEBUG_COMMENT_READER) {
        to_log(LOG_ERROR, "{s} at {get_line_info(1) |> describe}\n")
    }
    panic(s)
}

[macro_function]
def private get_macro_name(st : StructurePtr; mtype : string) {
    for (ann in st.annotations) {
        if (ann.annotation.name == mtype) {
            for (arg in ann.arguments) {
                if (arg.name == "name") {
                    return string(arg.sValue)
                }
            }
        }
    }
    return ""
}

[comment_reader]
class RstComment : AstCommentReader {
    chars : array<uint8>
    state : ParserState = ParserState.none
    comment : string
    structure_comment : string
    function_comment : string
    global_comment : string
    field_comment : string
    variant_comment : string
    variant_entry_comment : string
    tuple_comment : string
    tuple_entry_comment : string
    bitfield_comment : string
    bitfield_entry_comment : string
    enum_comment : string
    enum_entry_comment : string
    alias_comment : string
    field_comments : array<tuple<kind : string; name : string; comment : string>>
    def override open(prog : ProgramPtr; mod : Module?; cpp : bool; info : LineInfo) : void {
        assert(length(chars) == 0, "already open")
    }
    def override accept(prog : ProgramPtr; mod : Module?; ch : int; info : LineInfo) : void {
        chars |> push(uint8(ch))
    }
    def override close(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        if (!DEBUG_COMMENT_READER && (length(chars) < 1 || int(chars[0]) != '!')) {
            delete chars
            return
        }
        if (length(chars) > 1 && int(chars[1]) == ' ') {
            chars |> erase(1)
        }
        chars |> erase(0)
        chars |> push(uint8('\n'))
        comment = string(chars)
        delete chars
        if (state == ParserState.structure) {
            structure_comment += comment
        } elif (state == ParserState.func || state == ParserState.method) {
            function_comment += comment
        } elif (state == ParserState.field) {
            field_comment += comment
        } elif (state == ParserState.enum_entry) {
            enum_entry_comment += comment
        } elif (state == ParserState.global) {
            global_comment += comment
        } elif (state == ParserState.variant_alias) {
            variant_comment += comment
        } elif (state == ParserState.variant_entry) {
            variant_entry_comment += comment
        } elif (state == ParserState.tuple_alias) {
            tuple_comment += comment
        } elif (state == ParserState.tuple_entry) {
            tuple_entry_comment += comment
        } elif (state == ParserState.bitfield_alias) {
            bitfield_comment += comment
        } elif (state == ParserState.bitfield_entry) {
            bitfield_entry_comment += comment
        } elif (state == ParserState.enum_decl) {
            enum_comment += comment
        } elif (state == ParserState.enum_entry) {
            enum_entry_comment += comment
        } elif (state == ParserState.type_alias) {
            alias_comment += comment
        } else {
            debug_rst("uncategorized comment // {comment}")
        }
        comment = ""
    }
    def override beforeAlias(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeAlias")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.type_alias
    }
    def override afterAlias(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterAlias {name} // {alias_comment}")
        if (state != ParserState.type_alias) {
            panic_rst("expecting type_alias, got {state}")
        }
        if (alias_comment != "") {
            write_to_detail("typedef-{prog.thisModuleName}-{name}", alias_comment)
        }
        state = ParserState.none
        alias_comment = ""
    }
    def override beforeEnumeration(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeEnumeration")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.enum_decl
    }
    def override beforeEnumerationEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeEnumerationEntries")
        if (state != ParserState.enum_decl) {
            panic_rst("expecting enum_decl, got {state}")
        }
        state = ParserState.enum_entry
        enum_entry_comment = ""
    }
    def override afterEnumerationEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterEnumerationEntry {name} // {enum_entry_comment}")
        enum_entry_comment = ""
    }
    def override afterEnumerationEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterEnumerationEntries")
        if (state != ParserState.enum_entry) {
            panic_rst("expecting enum_entry, got {state}")
        }
        state = ParserState.enum_decl
    }
    def override afterEnumeration(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterEnumeration {name} // {enum_comment}")
        if (state != ParserState.enum_decl) {
            panic_rst("expecting enum_decl, got {state}")
        }
        if (enum_comment != "") {
            write_to_detail("enumeration-{prog.thisModuleName}-{name}", enum_comment)
        }
        state = ParserState.none
        enum_comment = ""
    }
    def override beforeBitfield(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeBitfield")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.bitfield_alias
    }
    def override beforeBitfieldEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeBitfieldEntries")
        if (state != ParserState.bitfield_alias) {
            panic_rst("expecting bitfield_alias, got {state}")
        }
        state = ParserState.bitfield_entry
        bitfield_entry_comment = ""
    }
    def override afterBitfieldEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterBitfieldEntry {name} // {bitfield_entry_comment}")
        if (state != ParserState.bitfield_entry) {
            panic_rst("expecting bitfield_entry, got {state}")
        }
        bitfield_entry_comment = ""
    }
    def override afterBitfieldEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterBitfieldEntries")
        state = ParserState.bitfield_alias
    }
    def override afterBitfield(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterBitfield {name} // {bitfield_comment}")
        if (state != ParserState.bitfield_alias) {
            panic_rst("expecting bitfield_alias, got {state}")
        }
        if (bitfield_comment != "") {
            write_to_detail("typedef-{prog.thisModuleName}-{name}", bitfield_comment)
        }
        state = ParserState.none
        bitfield_comment = ""
    }
    def override beforeTuple(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeTuple")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.tuple_alias
    }
    def override beforeTupleEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeTupleEntries")
        if (state != ParserState.tuple_alias) {
            panic_rst("expecting tuple_alias, got {state}")
        }
        state = ParserState.tuple_entry
        tuple_entry_comment = ""
    }
    def override afterTupleEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterTupleEntry {name} // {tuple_entry_comment}")
        if (state != ParserState.tuple_entry) {
            panic_rst("expecting tuple_entry, got {state}")
        }
        tuple_entry_comment = ""
    }
    def override afterTupleEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterTupleEntries")
        if (state != ParserState.tuple_entry) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.tuple_alias
    }
    def override afterTuple(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterTuple {name} // {tuple_comment}")
        if (state != ParserState.tuple_alias) {
            panic_rst("expecting tuple_alias, got {state}")
        }
        if (tuple_comment != "") {
            write_to_detail("typedef-{prog.thisModuleName}-{name}", tuple_comment)
        }
        state = ParserState.none
        tuple_comment = ""
    }
    def override beforeVariant(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeVariant")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.variant_alias
    }
    def override beforeVariantEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeVariantEntries")
        if (state != ParserState.variant_alias) {
            panic_rst("expecting variant_alias, got {state}")
        }
        state = ParserState.variant_entry
        variant_entry_comment = ""
    }
    def override afterVariantEntry(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterVariantEntry {name} // {variant_entry_comment}")
        if (state != ParserState.variant_entry) {
            panic_rst("expecting variant_entry, got {state}")
        }
        variant_entry_comment = ""
    }
    def override afterVariantEntries(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterVariantEntries")
        if (state != ParserState.variant_entry) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.variant_alias
    }
    def override afterVariant(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterVariant {name} // {variant_comment}")
        if (state != ParserState.variant_alias) {
            panic_rst("expecting variant_alias, got {state}")
        }
        if (variant_comment != "") {
            write_to_detail("typedef-{prog.thisModuleName}-{name}", variant_comment)
        }
        state = ParserState.none
        variant_comment = ""
    }
    def override beforeGlobalVariables(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeGlobalVariables")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.global
    }
    def override afterGlobalVariable(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterGlobalVariable {name} // {global_comment}")
        if (global_comment != "") {
            write_to_detail("variable-{prog.thisModuleName}-{name}", global_comment)
        }
    }
    def override afterGlobalVariables(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterGlobalVariables")
        if (state != ParserState.global) {
            panic_rst("expecting globl, got {state}")
        }
        state = ParserState.none
        global_comment = ""
    }
    def override beforeStructure(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeStucture")
        if (state != ParserState.none) {
            panic_rst("expecting none, got {state}")
        }
        state = ParserState.structure
    }
    def override afterStructure(st : StructurePtr; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterStucture {st._module.name}::{st.name} // {structure_comment}")
        if (state != ParserState.structure && state != ParserState.field) {
            panic_rst("expecting structure or field, got {state}")
        }
        state = ParserState.none
        var is_annotation = false
        if (structure_comment != "") {
            for (ann in st.annotations) {
                if (ann.annotation.name == "function_macro" || ann.annotation.name == "call_macro" || ann.annotation.name == "contract"
                    || ann.annotation.name == "block_macro" || ann.annotation.name == "tag_function_macro") {
                    var ann_name = get_macro_name(st, string(ann.annotation.name))
                    if (ann_name |> empty) {
                        ann_name = string(st.name)
                    }
                    write_to_detail("function_annotation-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                }
                if (ann.annotation.name == "structure_macro") {
                    let ann_name = get_macro_name(st, string(ann.annotation.name))
                    write_to_detail("structure_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                } elif (ann.annotation.name == "variant_macro") {
                    let ann_name = get_macro_name(st, "variant_macro")
                    write_to_detail("variant_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                } elif (ann.annotation.name == "typeinfo_macro") {
                    let ann_name = get_macro_name(st, "typeinfo_macro")
                    write_to_detail("typeinfo_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                } elif (ann.annotation.name == "reader_macro") {
                    let ann_name = get_macro_name(st, "reader_macro")
                    write_to_detail("reader_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                } elif (ann.annotation.name == "capture_macro") {
                    let ann_name = get_macro_name(st, "capture_macro")
                    write_to_detail("capture_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                } else {
                    to_log(LOG_WARNING, "WARNING: unsupported structure-annotation = {ann.annotation.name} in {st.name}\n")
                }
            }
        }
        // to_log(LOG_WARNING, "WARNING: unsupported structure-annotation = {ann.annotation.name} in {st.name}\n")
        if (!is_annotation && structure_comment != "") {
            let topic_name = st.flags.isClass ? "class" : "structure"
            write_to_detail("{topic_name}-{prog.thisModuleName}-{st.name}", structure_comment)
        }
        for (fc in field_comments) {
            if (fc.kind == "method") {
                write_to_detail("method-{prog.thisModuleName}-{st.name}.{fc.name}", fc.comment)
            }
            debug_rst("    {fc.kind} {fc.name} // {fc.comment}")
        }
        structure_comment = ""
        delete field_comments
    }
    def override beforeFunction(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeFunction")
        if (state != ParserState.none && state != ParserState.structure && state != ParserState.field) {
            panic_rst("expecting none, structure, or field got {state}")
        }
        state = state == ParserState.none ? ParserState.func : ParserState.method
    }
    def override afterFunction(fn : FunctionPtr; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterFunction {state} {prog.thisModuleName}::{fn.name} // {function_comment}")
        if (state != ParserState.func && state != ParserState.method) {
            panic_rst("expecting func or method, got {state}")
        }
        if (state == ParserState.func) {
            state = ParserState.none
            if (function_comment != "") {
                write_to_detail("function-{prog.thisModuleName}-{function_file_name(fn)}", function_comment)
            }
        } else {
            if (function_comment != "") {
                field_comments |> push(("method", string(fn.name), function_comment))
            }
            state = ParserState.structure
        }
        function_comment = ""
    }
    def override beforeStructureFields(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("beforeStuctureFields")
        if (state != ParserState.structure) {
            panic_rst("expecting structure, got {state}")
        }
        state = ParserState.field
    }
    def override afterStructureField(name : string; prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterStuctureField {name} // {field_comment}")
        if (state != ParserState.structure && state != ParserState.field) {
            panic_rst("expecting structure or field, got {state}")
        }
        state = ParserState.field
        if (field_comment != "") {
            debug_rst("\t-> push {name} // {field_comment}")
            field_comments |> push(("field", clone_string(name), field_comment))
        }
    }
    def override afterStructureFields(prog : ProgramPtr; mod : Module?; info : LineInfo) : void {
        debug_rst("afterStuctureFields")
        if (state != ParserState.structure && state != ParserState.field) {
            panic_rst("expecting structure or field, got {state}")
        }
        state = ParserState.structure
        field_comment = ""
    }
    def write_to_detail(name, text : string) : void {
        if (is_in_aot() || is_in_completion()) {
            return
        }
        if (DEBUG_COMMENT_READER) {
            to_log(LOG_INFO, "write_to_detail {name}\n")
        }
        let fname = get_das_root() + "/doc/source/stdlib/detail/{name}.rst"
        fopen(fname, "wb") <| $(f) {
            if (f == null) {
                to_log(LOG_ERROR, "can't open {fname}\n")
            } else {
                f |> fwrite(text)
            }
        }
    }
}






