// Regression for issue #3065 (fuzzer, AOT): binding a local reference to a temporary
// value -- here a freshly-built array `[1]` -- has no addressable storage to point at,
// so the reference would dangle. The front-end used to let this through under `unsafe`
// (in the fuzzer it was the finally of an unsafe block that suppressed the check), after
// which it crashed the interpreter at runtime and emitted ill-formed C++ (`&temporary`)
// under AOT. It is now a hard compile error regardless of `unsafe`.
// A view built ON the temporary is still the temporary (a cast, or the `??` default arm), and
// the expression form of `unsafe` marks the initializer alwaysSafe -- none of the three may
// suppress the check.
options gen2
expect 31019 : 4

struct Bag {
    xs : array<int>
}

def private make_bag : Bag {
    var b : Bag
    b.xs |> push(7)
    return <- b
}

[export]
def main {
    unsafe {
        let r & := [1]
        let viaCast & = reinterpret<Bag&>(make_bag())
        var p : Bag? = null
        let viaDefault & = p ?? make_bag()
    }
    let viaUnsafeExpr & = unsafe(make_bag())
}
