// Fixture for test_inline_errors.das — intentionally fails to compile: [inline]
// on a punctuation function whose call sites are NOT ExprOp nodes (operator []
// dispatches through ExprAt), so the splicer can't honor the contract.
// ExprOp-dispatched operator overloads (+, -, ==, +=, ...) inline fine.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 50501

options gen2

struct SVec {
    x : int;
    y : int;
}

[inline]
def operator [] (sv : SVec; index : int) : int {
    return index == 0 ? sv.x : sv.y;
}

[export]
def main() {
    let a = SVec(x = 1, y = 2);
    let c = a[0];
    feint("{c}\n");
}
