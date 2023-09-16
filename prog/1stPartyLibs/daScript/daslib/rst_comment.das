options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module rst_comment private

require daslib/ast_boost
require fio
require strings

enum ParserState
    none
    structure
    field
    func
    method
    global
    variant_alias
    enum_decl
    type_alias

let private DEBUG_COMMENT_READER = false

[macro_function]
def private debug_rst ( s : string )
    if DEBUG_COMMENT_READER
        to_log(LOG_INFO,s)

[macro_function]
def private get_macro_name ( st:StructurePtr; mtype:string )
    for ann in st.annotations
        if ann.annotation.name==mtype
            for arg in ann.arguments
                if arg.name=="name"
                    return string(arg.sValue)
    return ""

[comment_reader]
class RstComment : AstCommentReader
    chars : array<uint8>
    state : ParserState = ParserState none
    comment : string
    structure_comment : string
    function_comment : string
    global_comment : string
    field_comment : string
    variant_comment : string
    enum_comment : string
    alias_comment : string
    field_comments : array<tuple<kind:string;name:string;comment:string>>
    def override open ( prog:ProgramPtr; mod:Module?; cpp:bool; info:LineInfo ) : void
        assert(length(chars)==0,"already open")
    def override accept ( prog:ProgramPtr; mod:Module?; ch:int; info:LineInfo ) : void
        chars |> push(uint8(ch))
    def override close ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        if !DEBUG_COMMENT_READER && (length(chars)<1 || int(chars[0])!='!')
            delete chars
            return
        if length(chars)>1 && int(chars[1])==' '
            chars |> erase(1)
        chars |> erase(0)
        chars |> push(uint8('\n'))
        comment = string(chars)
        delete chars
        if state==ParserState structure
            structure_comment += comment
        elif state==ParserState func || state==ParserState method
            function_comment += comment
        elif state==ParserState field
            field_comment += comment
        elif state==ParserState global
            global_comment += comment
        elif state==ParserState variant_alias
            variant_comment += comment
        elif state==ParserState enum_decl
            enum_comment += comment
        elif state==ParserState type_alias
            alias_comment += comment
        else
            debug_rst("uncategorized comment // {comment}")
        comment = ""
    def override beforeAlias ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeAlias")
        if state != ParserState none
            panic("expecting none, got {state}")
        state = ParserState type_alias
    def override afterAlias ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterAlias {name} // {alias_comment}")
        if state != ParserState type_alias
            panic("expecting type_alias, got {state}")
        if alias_comment != ""
            write_to_detail("typedef-{prog.thisModuleName}-{name}", alias_comment)
        state = ParserState none
        alias_comment = ""
    def override beforeEnumeration ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeEnumeration")
        if state != ParserState none
            panic("expecting none, got {state}")
        state = ParserState enum_decl
    def override afterEnumeration ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterEnumeration {name} // {enum_comment}")
        if state != ParserState enum_decl
            panic("expecting enum_decl, got {state}")
        if enum_comment != ""
            write_to_detail("enumeration-{prog.thisModuleName}-{name}", enum_comment)
        state = ParserState none
        enum_comment = ""
    def override beforeVariant ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeVariant")
        if state != ParserState none
            panic("expecting none, got {state}")
        state = ParserState variant_alias
    def override afterVariant ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterVariant {name} // {variant_comment}")
        if state != ParserState variant_alias
            panic("expecting variant_alias, got {state}")
        if variant_comment != ""
            write_to_detail("typedef-{prog.thisModuleName}-{name}", variant_comment)
        state = ParserState none
        variant_comment = ""
    def override beforeGlobalVariables ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeGlobalVariables")
        if state != ParserState none
            panic("expecting none, got {state}")
        state = ParserState global
    def override afterGlobalVariable ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterGlobalVariable {name} // {global_comment}")
        if global_comment != ""
            write_to_detail("variable-{prog.thisModuleName}-{name}", global_comment)
    def override afterGlobalVariables ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterGlobalVariables")
        if state != ParserState global
            panic("expecting globl, got {state}")
        state = ParserState none
        global_comment = ""
    def override beforeStructure ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeStucture")
        if state != ParserState none
            panic("expecting none, got {state}")
        state = ParserState structure
    def override afterStructure ( st:StructurePtr; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterStucture {st._module.name}::{st.name} // {structure_comment}")
        if state!=ParserState structure && state!=ParserState field
            panic("expecting structure or field, got {state}")
        state = ParserState none
        var is_annotation = false
        if structure_comment != ""
            for ann in st.annotations
                if (ann.annotation.name=="function_macro" || ann.annotation.name=="call_macro" || ann.annotation.name=="contract"
                    || ann.annotation.name=="block_macro" || ann.annotation.name=="tag_function_macro")
                    let ann_name = get_macro_name(st,string(ann.annotation.name))
                    write_to_detail("function_annotation-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                if ann.annotation.name=="structure_macro"
                    let ann_name = get_macro_name(st,string(ann.annotation.name))
                    write_to_detail("structure_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                elif ann.annotation.name=="variant_macro"
                    let ann_name = get_macro_name(st,"variant_macro")
                    write_to_detail("variant_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                elif ann.annotation.name=="typeinfo_macro"
                    let ann_name = get_macro_name(st,"typeinfo_macro")
                    write_to_detail("typeinfo_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                elif ann.annotation.name=="reader_macro"
                    let ann_name = get_macro_name(st,"reader_macro")
                    write_to_detail("reader_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                elif ann.annotation.name=="capture_macro"
                    let ann_name = get_macro_name(st,"capture_macro")
                    write_to_detail("capture_macro-{prog.thisModuleName}-{ann_name}", structure_comment)
                    is_annotation = true
                    break
                else
                    to_log(LOG_WARNING, "WARNING: unsupported structure-annotation = {ann.annotation.name} in {st.name}\n")
        // to_log(LOG_WARNING, "WARNING: unsupported structure-annotation = {ann.annotation.name} in {st.name}\n")
        if !is_annotation && structure_comment != ""
            let topic_name = st.flags.isClass ? "class" : "structure"
            write_to_detail("{topic_name}-{prog.thisModuleName}-{st.name}", structure_comment)
        for fc in field_comments
            if fc.kind=="method"
                write_to_detail("method-{prog.thisModuleName}-{st.name}.{fc.name}", fc.comment)
            debug_rst("    {fc.kind} {fc.name} // {fc.comment}")
        structure_comment = ""
        delete field_comments
    def override beforeFunction ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeFunction")
        if state!=ParserState none && state!=ParserState structure && state!=ParserState field
            panic("expecting none, structure, or field got {state}")
        state = state==ParserState none ? ParserState func : ParserState method
    def override afterFunction ( fn:FunctionPtr; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterFunction {state} {prog.thisModuleName}::{fn.name} // {function_comment}")
        if state!=ParserState func && state!=ParserState method
            panic("expecting func or method, got {state}")
        if state==ParserState func
            state = ParserState none
            if function_comment != ""
                write_to_detail("function-{prog.thisModuleName}-{fn.name}", function_comment)
        else
            if function_comment != ""
                field_comments |> push([[auto "method",string(fn.name),function_comment]])
            state = ParserState structure
        function_comment = ""
    def override beforeStructureFields ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("beforeStuctureFields")
        if state!=ParserState structure
            panic("expecting structure, got {state}")
        state = ParserState field
    def override afterStructureField ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterStuctureField {name} // {field_comment}")
        if state!=ParserState structure && state!=ParserState field
            panic("expecting structure or field, got {state}")
        state = ParserState field
        if field_comment != ""
            debug_rst("\t-> push {name} // {field_comment}")
            field_comments |> push([[auto "field",clone_string(name),field_comment]])
    def override afterStructureFields ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        debug_rst("afterStuctureFields")
        if state!=ParserState structure && state!=ParserState field
            panic("expecting structure or field, got {state}")
        state = ParserState structure
        field_comment = ""
    def write_to_detail ( name, text : string ) : void
        let fname = get_das_root() + "/doc/source/stdlib/detail/{name}.rst"
        fopen(fname,"wb") <| $ ( f )
            if f==null
                panic("can't open {fname}")
            else
                f |> fwrite(text)






