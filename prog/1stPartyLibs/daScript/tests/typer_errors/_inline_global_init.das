// Fixture for test_inline_errors.das — intentionally fails to compile: [inline]
// call in a global initializer. `_`-prefix keeps dastest from auto-running it;
// the `expect` directive makes lint skip it.
expect 30100

options gen2

[inline]
def add(a, b : int) : int {
    return a + b;
}

var gg = add(1, 2)

[export]
def main() {
    feint("{gg}\n");
}
