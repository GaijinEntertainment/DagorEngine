// Regression guard: `throw null` is a control-flow sentinel (clean failure /
// skip), not an error. The async value-type trace capture added a branch at the
// universal exception_trap that scans for an async root; the sentinel exclusion
// is that a null whose nearest root is a non-async callback (these run under a
// nested ET_CALL) is treated as the skip, never an escaping fault. The sync
// cases below must be unaffected; async_throw_null_sentinel.nut guards the same
// sentinels when an async step is on the stack.

// .map: a callback that throws null skips that element.
let evens = [1, 2, 3, 4, 5, 6].map(function(x) {
    if (x % 2 == 0)
        return x
    throw null
})
println(" ".join(evens))

// _get throwing null means "not found" and falls through (here, to an index
// error), rather than propagating null as a thrown value.
class Proxy {
    function _get(key) {
        if (key == "x")
            return 99
        throw null
    }
}

let t = Proxy()
println(t.x)
try {
    let _unused = t.missing
    println("no error")
} catch (e) {
    println("missing raised")
}
