require ast
require rtti
require daslib/ast_boost

[export]
def main
    using() <| $ ( var lib : ModuleGroup )
        var p <- parse_mangled_name("CN<;f>0<i;f>U",lib,null)
        debug(p)
        print("{describe(p)}\n")
        p := null


