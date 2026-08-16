// Fixture for test_inline_errors.das — intentionally fails to compile: an [inline]
// operator whose body reaches itself through an operator SITE (not a plain call) -
// the inline-graph recursion scan must see ExprOp edges too.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 50501

options gen2

struct SVec {
    x : int;
}

[inline]
def operator ^(a, b : SVec) : SVec {
    return SVec(x = a.x + 1) ^ b;
}

[export]
def main() {
    let a = SVec(x = 1);
    let b = SVec(x = 2);
    let c = a ^ b;
    feint("{c.x}\n");
}
