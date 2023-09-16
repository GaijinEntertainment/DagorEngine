options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module refactor shared private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/json_boost
require daslib/defer
require strings

let LOG_REFACTOR = 100500

[tag_function(extract_method_tag),sideeffects]
def public extract_method ( method_name:string; blk:block )
    invoke(blk)

[tag_function(extract_method_tag),sideeffects]
def public extract_expression ( method_name:string; expr )
    return expr

def log_range ( at:LineInfo )
    return "{describe(at)}-{int(at.last_line)}:{int(at.last_column)}"

struct ExtractMethodDesc
    call_name : string
    function_declaration : string
    call_expression : string
    call_at : LineInfo
    function_body_at : LineInfo

def JV ( li:LineInfo )
    let fname = li.fileInfo!=null ? string(li.fileInfo.name) : ""
    let tabSize = li.fileInfo?.tabSize ?? 4
    var inscope kv <- {{
        "line" => "{int(li.line)}";
        "column" => "{int(li.column)}";
        "last_line" => "{int(li.last_line)}";
        "last_column" => "{int(li.last_column)}";
        "file" => fname;
        "tab" => "{tabSize}"
    }}
    return JV(kv)

[tag_function_macro(tag="extract_method_tag")]
class ExtractMethodMacro : AstFunctionAnnotation
    method_tag = "!!!extract_method"
    def override verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        if !(call.arguments[0] is ExprConstString)
            print("extract_method: first argument must be a string literal")
            return false
        var capture <- capture_block(call.arguments[1])
        sort(capture) <| $ ( a,b ) => string(a.variable.name) < string(b.variable.name)
        var call_name = string((call.arguments[0] as ExprConstString).value)
        let function_declaration = build_string <| $ ( writer )
            writer |> write ( "def ")
            writer |> write ( call_name )
            if length(capture) != 0
                writer |> write ( " ( " )
                var first = true
                for v in capture
                    if first
                        first = false
                    else
                        writer |> write("; ")
                    if !v.variable._type.flags.constant && v.variable.access_flags.access_ref
                        writer |> write ( "var " )
                    writer |> write ( v.variable.name )
                    writer |> write ( ":")
                    writer |> write ( describe(v.variable._type) )
                    if v.variable._type.isWorkhorseType && v.variable.access_flags.access_ref
                        writer |> write("&")
                writer |> write ( " )" )

        let call_expression = build_string <| $ ( writer )
            writer |> write ( call_name )
            writer |> write ( "(" )
            if length(capture) != 0
                var first = true
                for v in capture
                    if first
                        first = false
                    else
                        writer |> write(",")
                    writer |> write ( v.variable.name )
            writer |> write ( ")" )
        assume atCall = call.atEnclosure
        assume atBlock = call.arguments[1].at
        var jv <- JV([[ExtractMethodDesc
            call_name = call_name,
            function_declaration = function_declaration,
            call_expression = call_expression,
            call_at = atCall,
            function_body_at = atBlock
        ]])
        to_compilation_log("{method_tag}\n{write_json(jv)}\n!!!\n")
        unsafe
            delete jv
        return true

struct ExtractVariableDesc
    variable_name : string
    variable_type : string
    variable_keyword : string
    is_reference : bool
    call_at : LineInfo
    variable_init_at : LineInfo

[tag_function(extract_variable_tag),sideeffects]
def public extract_variable_nonref ( method_name:string; expr )
    return expr

[tag_function(extract_variable_tag),sideeffects]
def public extract_variable_ref ( method_name:string; var expr:auto(TT)& ==const ) : TT&
    unsafe
        return expr

[call_macro(name="extract_variable")]
class ExtractVariableMacro : AstCallMacro
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        if !(expr.arguments[0] is ExprConstString)
            macro_error(prog,expr.at,"extract_variable: first argument must be a string literal")
            return <- [[ExpressionPtr]]
        assume varType = expr.arguments[1]._type
        if varType==null || varType.isAutoOrAlias
            macro_error(prog,expr.at,"extract_variable: unresolve expression type")
            return <- [[ExpressionPtr]]
        var inscope ccall <- new [[ExprCall() at=expr.at]]
        ccall.name := (varType.isRef && !varType.isConst) ? "extract_variable_ref" : "extract_variable_nonref"
        ccall.arguments |> emplace_new <| clone_expression(expr.arguments[0])
        ccall.arguments |> emplace_new <| clone_expression(expr.arguments[1])
        return <- ccall

[tag_function_macro(tag="extract_variable_tag")]
class ExtractVariableFunction : AstFunctionAnnotation
    method_tag = "!!!extract_variable"
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        assume cffg = call.func.fromGeneric
        assume varType = call.arguments[1]._type
        if call.func!=null && cffg!=null && cffg.name=="extract_variable"
            if varType.isConst || !varType.isRef
                var inscope ccall <- clone_expression(call)
                (ccall as ExprCall).name := "extract_variable_nonref"
                return <- ccall
        return <- [[ExpressionPtr]]
    def override verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        if !(call.arguments[0] is ExprConstString)
            print("extract_variable: first argument must be a string literal")
            return false
        assume varType = call.arguments[1]._type
        var variable_name = string((call.arguments[0] as ExprConstString).value)
        assume atCall = call.atEnclosure
        assume atBlock = call.arguments[1].at
        var jv <- JV([[ExtractVariableDesc
            variable_name = variable_name,
            call_at = atCall,
            variable_type = describe(varType),
            variable_keyword = varType.isConst ? "let" : "var",
            is_reference = varType.isRef && !varType.isConst,
            variable_init_at = atBlock
        ]])
        to_compilation_log("{method_tag}\n{write_json(jv)}\n!!!\n")
        unsafe
            delete jv
        return true

