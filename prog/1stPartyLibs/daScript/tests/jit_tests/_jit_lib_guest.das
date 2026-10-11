options gen2


require daslib/ast
require daslib/export_c
require UnitTest

[export_c]
def probe(n : int) : int {
    return n + 7
}

[export_c]
def touch_ast : bool {
    return compiling_module() == null
}
