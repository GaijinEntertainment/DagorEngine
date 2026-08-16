// Fixture for test_inline_errors.das — intentionally fails to compile: [inline]
// (a fail-closed must-splice contract) and [never_inline] (opt-out of all
// best-effort splicing) are contradictory on one function.
expect 20800

options gen2

[inline, never_inline]
def bad(x : int) : int {
    return x + 1;
}

[export]
def main() {
    let y = bad(1);
    print("{y}\n");
}
