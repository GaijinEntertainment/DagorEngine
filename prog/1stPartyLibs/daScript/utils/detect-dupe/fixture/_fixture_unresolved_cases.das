options gen2

require daslib/ast
require daslib/no_such_module


[no_such_annotation]
def case_unresolved_annotated(a, b : int) : int {
    return a + b
}


class CaseUnresolvedParent : NoSuchParent {
    def override visit(a : int) : int {
        return a * 2
    }
}
