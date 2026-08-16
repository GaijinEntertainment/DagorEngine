// Fixture for test_inline_errors.das — intentionally fails to compile: mutual
// recursion through the inline graph. `_`-prefix keeps dastest from auto-running
// it; the `expect` directive makes lint skip it.
expect 50501

options gen2

[inline]
def pingf(x : int) : int {
    return x <= 0 ? 0 : pongf(x - 1);
}

[inline]
def pongf(x : int) : int {
    return x <= 0 ? 1 : pingf(x - 1);
}

[export]
def main() {
    let y = pingf(4);
    feint("{y}\n");
}
