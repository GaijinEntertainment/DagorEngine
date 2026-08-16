// Fixture for test_inline_errors.das — intentionally fails to compile: a
// STATEMENT-body [inline] callee in a mixed-type ternary arm (Node? vs null).
// The lazy lowering would split the ternary into bare arm stores, losing the
// per-arm null-to-pointer coercion, so the fail-closed contract errors instead.
// (An expression-body callee pure-grafts into the intact ternary and is fine.)
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 30100

options gen2

struct Node {
    v : int;
}

[inline]
def get_node(v : int) : Node? {
    var n = new Node(v = v);
    n.v++;
    return n;
}

def pick(f : bool; v : int) : int {
    var p = f ? get_node(v) : null;
    let r = p != null ? p.v : -1;
    unsafe {
        delete p;
    }
    return r;
}

[export]
def main() {
    print("{pick(true, 3)}\n");
}
