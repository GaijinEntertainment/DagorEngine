// Fixture for test_inline_errors.das — intentionally fails to compile: [inline]
// call in a struct field initializer. `_`-prefix keeps dastest from auto-running
// it; the `expect` directive makes lint skip it.
expect 30100

options gen2

[inline]
def add(a, b : int) : int {
    return a + b;
}

struct Foo {
    x : int = add(1, 2);
}

[export]
def main() {
    let f = Foo();
    feint("{f.x}\n");
}
