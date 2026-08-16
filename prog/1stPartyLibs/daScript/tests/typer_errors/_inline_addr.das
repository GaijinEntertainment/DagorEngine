// Fixture for test_inline_errors.das — intentionally fails to compile: @@ of an
// [inline] function. `_`-prefix keeps dastest from auto-running it; the `expect`
// directive makes lint skip it.
expect 30100

options gen2

[inline]
def add(a, b : int) : int {
    return a + b;
}

[export]
def main() {
    let p = @@add;
    let y = invoke(p, 1, 2);
    feint("{y}\n");
}
