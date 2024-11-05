options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module spoof shared private

require daslib/ast_boost
require peg/peg
require strings

let SPOOF_SPEFICAL = '%'

[macro_function]
def spoof_args ( invocation:string; blk:block<(var res : array<tuple<string;string>>; errors:array<ParsingError>):void> )
    parse invocation
        var element_list : array<tuple<string;string>>
        rule(WS, "(", WS, *comma_separated_elements as els, WS, element as last, WS, ")") <|
            els |> push <| last
            return <- els
        var comma_separated_elements : tuple<string;string>
        rule(WS,element as e, WS, ",") <|
            return e
        var element : tuple<string;string>
        rule(ident_ as e, WS, "=", WS, element_value as v) <|
            return [[auto e, v]]
        rule(ident_ as e) <|
            return [[auto e, ""]]
        var sub_element : string
        rule("\\", "{set(0..255)}" as Ch) <|    // escape
            return string(Ch)
        rule("#",string_ as e,commit) <|        // contents of the string (do we need unescape?)
            return e
        rule(string_ as e,commit) <|            // "e"
            return "\"{e}\""
        rule("(",element as e,")") <|           // (e)
            return "({e})"
        rule("{not_set(')',',')}" as e) <|      // any character except ')' and ','
            return e
        var element_value : string
        rule(sub_element as head, *sub_element as tail) <|
            return head + tail |> join("")
        var ident_ : string
        rule("{set('a'..'z','A'..'Z','_')}" as fc,"{*set('a'..'z','A'..'Z','_','0'..'9')}" as tail) <|
            return fc + tail

[macro_function]
def next_ident ( it:iterator<int>; var Ch:int&; var ident:string& ) : bool
    var id : array<uint8>
    Ch = -1
    while next(it,Ch)
        if is_alpha(Ch) || is_number(Ch) || Ch=='_'
            id |> push(uint8(Ch))
        else
            break
    return false if id |> length == 0
    ident = string(id)
    return true

[macro_function]
def instance_spoof ( temp:string; passed_arguments:array<string> ) : tuple<result:string;ok:bool>
    var args : array<tuple<argName:string;init:string>>
    var error = "no arguments found"
    spoof_args(temp) <| $ ( res, errors )
        if errors |> length > 0
            error = errors |> join("\n")
        else
            args <- res
    if args |> length == 0
        return [[auto error, false]]
    // compare argument counts
    var instance_arguments : array<string>
    for a,ai in args,count()
        if ai < passed_arguments |> length
            instance_arguments |> push <| passed_arguments[ai]
        elif !empty(a.init)
            instance_arguments |> push <| a.init
        else
            return [[auto "invalid number of arguments", false]]
    // skip to next line
    var it <- unsafe(each(temp))
    var Ch = -1
    while next(it,Ch)
        if  Ch=='\n'
            break
        elif Ch=='\r'
            pass
        Ch = -1
    if Ch==-1
        return [[auto "unexpected end of file", false]]
    // now, lets apply the arguments
    var result : array<uint8>
    var pendingCh : int
    Ch = 0
    while Ch==-1 || next(it,Ch)
        if Ch==-1                   // we have pending character
            Ch = pendingCh
        if Ch==SPOOF_SPEFICAL
            var ident : string
            if next_ident(it,Ch,ident)
                var index = args |> find_index_if <| $ ( k )
                    return k.argName == ident
                if index != -1
                    for c in instance_arguments[index]
                        result |> push(uint8(c))
                if Ch!=-1           // pending after ident
                    pendingCh = Ch
                    Ch = -1
            elif Ch==SPOOF_SPEFICAL
                result |> push(uint8(SPOOF_SPEFICAL))
                result |> push(uint8(SPOOF_SPEFICAL))
            elif Ch==-1
                break   // eof
            else
                return [[auto "invalid sequence after %", false]]
        else
            result |> push(uint8(Ch))

    return [[auto string(result), true]]


[reader_macro(name="spoof_template")]
class SpoofTemplateReader : AstReaderMacro
    def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
        append(expr.sequence,ch)
        if ends_with(expr.sequence,"%%")
            let len = length(expr.sequence)
            resize(expr.sequence,len-2)
            return false
        else
            return true
    def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
        return <- new [[ExprConstString() at=expr.at, value:=expr.sequence]]

struct SpoofInvocation
    varName : string
    args : array<string>

def instance_args ( invocation:string; blk:block<(var res : SpoofInvocation; errors:array<ParsingError>):void> )
    parse invocation
        var macro_invocation : SpoofInvocation
        rule(WS, class_name as cn, WS, "(", element_list as el, WS, ")") <|
            return <- [[SpoofInvocation varName=cn, args <- el]]
        rule(WS, class_name as cn, WS, "(", WS, ")") <|
            return <- [[SpoofInvocation varName=cn]]
        var element_list : array<string>
        rule(*comma_separated_elements as els, WS, element as last) <|
            els |> push <| last
            return <- els
        var comma_separated_elements : string
        rule(WS,element as e, ",") <|
            return e
        var sub_element : string
        rule("\\", "{set(0..255)}" as Ch) <|    // escape
            return string(Ch)
        rule("#",string_ as e,commit) <|        // contents of the string (do we need unescape?)
            return e
        rule(string_ as e,commit) <|            // "e"
            return "\"{e}\""
        rule("(",element as e,")") <|           // (e)
            return "({e})"
        rule("{not_set(')',',')}" as e) <|      // any character except ')' and ','
            return e
        var element : string
        rule(sub_element as head, *sub_element as tail) <|
            return head + tail |> join("")
        var class_name : string
        rule(WS,"{ident_}" as scope, "::", "{ident_}" as id) <|
            return scope + "::" + id
        rule(WS,ident_ as id) <|
            return id
        var ident_ : string
        rule("{set('a'..'z','A'..'Z','_')}" as fc,"{*set('a'..'z','A'..'Z','_','0'..'9')}" as tail) <|
            return fc + tail

[reader_macro(name="spoof_instance")]
class SpoofInstanceReader : AstReaderMacro
    def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
        append(expr.sequence,ch)
        if ends_with(expr.sequence,"%%")
            let len = length(expr.sequence)
            resize(expr.sequence,len-2)
            return false
        else
            return true
    def override suffix ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; info:LineInfo; var outLine:int&; var outFile:FileInfo?& ) : string
        outLine = int(info.line)
        var iargs : SpoofInvocation
        var failed = false
        instance_args(string(expr.sequence)) <| $ ( res, errors )
            if errors |> length > 0
                let all_errors = errors |> join("\n")
                prog |> macro_error(info, all_errors)
                failed = true
            else
                iargs <- res
        return "" if failed
        // lets find the variable for the template
        var inscope vptr : VariablePtr
        find_matching_variable(get_ptr(prog),null,iargs.varName,false) <| $ ( vars )
            if vars |> length == 0
                prog |> macro_error(info, "no matching variable found for {iargs.varName}")
                return
            elif vars |> length > 1
                prog |> macro_error(info, "multiple matching variables found for {iargs.varName}")
                return
            else
                vptr := vars[0]
        if vptr == null
            return ""
        if vptr.init == null
            prog |> macro_error(info, "variable {iargs.varName} has no initializer")
            return ""
        var vstr = ""
        if vptr.init is ExprConstString
            vstr = string((vptr.init as ExprConstString).value)
        elif vptr.init is ExprReader
            vstr = string((vptr.init as ExprReader).sequence)
        else
            prog |> macro_error(info, "variable {iargs.varName} is not a string")
            return ""
        // now, lets spoof the instance
        outLine = int(vptr.init.at.line) + 1
        outFile = vptr.init.at.fileInfo
        var result <- instance_spoof(string(vstr),iargs.args)
        if !result.ok
            prog |> macro_error(info, "spoof instance error {result.result}")
            return ""
        return result.result

