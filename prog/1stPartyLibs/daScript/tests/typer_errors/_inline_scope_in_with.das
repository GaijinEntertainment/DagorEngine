// an [inline] call site inside `with (module ...)` naming a DIFFERENT module than
// the callee's origin: the enclosing scope is the resolution context, and a bare
// splice would re-home the body's names there - fail-closed.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 30100
options gen2
require _inline_scope_dispatch_helper

[inline]
def local_add(a, b : int) : int {
    return a + b
}

[export]
def main() {
    var r = 0
    with (module _inline_scope_dispatch_helper) {
        r = _::local_add(1, 2)
    }
    print("{r}\n")
}
