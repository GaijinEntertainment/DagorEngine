// Fixture for test_static_if_profiles.das — two const globals whose initializers
// reference each other through an operator. getConstExpr's in-flight set must break
// the cycle so the standard init-loop diagnostic reports it (this shape SIGSEGV'd
// when the recursion arm first landed).
expect 31104

options gen2

let A : int = B + 1
let B : int = A + 1

[export]
def main() {
    print("{A}\n")
}
