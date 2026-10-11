// Fixture for test_inline_errors.das — intentionally fails to compile: an
// [inline] callee takes a writable by-ref parameter, and the argument is a
// ternary of two lvalues. A ternary SELECTS an arm rather than producing a
// fresh value, so binding the parameter to a by-value temp would drop the
// callee's write; the fail-closed contract errors instead of corrupting.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 30100

options gen2

struct Plain {
    n : int = 1;
}

[inline]
def poke_n(var s : Plain) {
    s.n = 42;
}

def pick(c : bool; var a : Plain; var b : Plain) {
    poke_n(c ? a : b);
}

[export]
def main() {
    var a = Plain();
    var b = Plain();
    pick(true, a, b);
    print("{a.n} {b.n}\n");
}
