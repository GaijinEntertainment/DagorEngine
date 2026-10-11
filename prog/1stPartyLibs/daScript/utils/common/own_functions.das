options gen2

require daslib/ast
require daslib/ast_boost
require daslib/fio
require strings


def private normalized(p : string) : string {
    return normalize(p) |> replace("\\", "/")
}


def private is_own(func : FunctionPtr; file_norm : string; var seen : table<string; void?>) : bool {
    if (func.flags.builtIn || func.flags.generated || func.at.fileInfo == null
            || normalized(string(func.at.fileInfo.name)) != file_norm) return false
    let loc = "{int(func.at.line)}:{int(func.at.column)}"
    if (key_exists(seen, loc)) return false
    seen |> insert(loc, null)
    return true
}


// Every function and generic template the program's own module declares in `file` - not a
// builtin, not generated, not one a required module contributed - once per `line:column`: a
// template and the instances reified from it share a site, and the first wins.
def public for_each_own_function(program : ProgramPtr; file : string; blk : block<(func : FunctionPtr) : void>) {
    let file_norm = normalized(file)
    var seen : table<string; void?>
    let thisMod = get_this_module(program)
    for_each_generic(thisMod, "") $(func) {
        if (is_own(func, file_norm, seen)) {
            invoke(blk, func)
        }
    }
    for_each_function(thisMod, "") $(func) {
        if (is_own(func, file_norm, seen)) {
            invoke(blk, func)
        }
    }
}
