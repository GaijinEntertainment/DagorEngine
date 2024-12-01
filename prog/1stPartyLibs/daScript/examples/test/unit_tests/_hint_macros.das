module _hint_macros

require ast
require rtti
require daslib/ast_boost public

struct HintLookup
    tab : table<uint64; string>

var
    g_non_hints = 0

def get_hint ( lookup : HintLookup; id, defv : string; hint : uint64 )
    return lookup.tab ?[ hint ] ?? defv

[tag_function(get_hint_tag)]
def get_hint ( lookup : HintLookup; id, defv : string )
    g_non_hints ++
    return get_hint(lookup,id,defv,hash(id))

[tag_function_macro(tag="get_hint_tag")]
class GetHintFnMacro : AstFunctionAnnotation
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        if call.arguments[1] is ExprConstString
            unsafe
                var new_call := call // <- clone_expression(call)
                let arg2 = reinterpret<ExprConstString?>(call.arguments[1])
                let hint = hash("{arg2.value}")
                emplace_new(new_call.arguments, new [[ExprConstUInt64() at=arg2.at, value=hint]])
                return new_call
        return <-[[ExpressionPtr]]


