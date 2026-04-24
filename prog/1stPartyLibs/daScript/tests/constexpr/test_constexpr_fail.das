options gen2
expect 40102:1

// Verify that [constexpr] annotation rejects non-constant arguments.
// The call `foo("ouch", BOO)` must fail because BOO is a global variable,
// not a compile-time constant.

require daslib/constant_expression

[constexpr(a)]
def foo(t : string; a : int) {
    feint("{t} = {a}\n")
}

var BOO = 13

[export]
def main() {
    foo("blah", 1)      // ok — literal 1 is constexpr
    foo("ouch", BOO)    // 40102: a is not constexpr
}
