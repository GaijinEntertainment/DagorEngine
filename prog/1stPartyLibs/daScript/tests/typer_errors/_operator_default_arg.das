// Fixture for test_operator_errors.das — intentionally fails to compile: a defaulted
// operand no operator call site can supply (or omit) crashes simulation.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 30161
options gen2

struct Foo {
    v : int
}

def operator +(a, b : Foo; scale : int = 1) {
    return Foo(v = (a.v + b.v) * scale)
}

[export]
def main() {
    let r = Foo(v = 1) + Foo(v = 2)
    print("r={r.v}\n")
}
