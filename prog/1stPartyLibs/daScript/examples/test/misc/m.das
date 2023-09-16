module m

require daslib.apply
require macros
require strings
require b
require daslib.ast_boost
require rtti

def handleDuplicateTransaction()
    var i = 0
    apply(p) <| $(name : string; field)
        if typeinfo(is_table field)
            for key in keys(field)
                print("{key}")
                i = i + length(key)
    return i > 5


def crashThere(field : auto(T))
    var c : T
    for k, v in keys(field), values(field)
        c[k] := v
    print("{c}")


def makeDeltaTransaction(profile)
    var i = 0
    apply(profile) <| $(name : string; field)
        static_if typeinfo(is_table field)
            crashThere(field)

[function_macro(name="unique_transaction")]
class UniqueTransaction : AppendCondAnnotation
    def override getCondName() : string
        return "handleDuplicateTransaction"
