// Fixture for test_inline_errors.das — intentionally fails to compile: a
// statement-needing [inline] call in a ?? default has no v1 lowering. `_`-prefix
// keeps dastest from auto-running it; the `expect` directive makes lint skip it.
expect 30100

options gen2

var g_count = 0

def side(v : int) : int {
    g_count += 1;
    return v;
}

[inline]
def add(a, b : int) : int {
    return a + b;
}

[export]
def main() {
    var q = 5;
    var p : int? = unsafe(addr(q));
    let z = p ?? add(1, side(2)); // statement-needing inline call in the ?? default
    feint("{z}\n");
}
