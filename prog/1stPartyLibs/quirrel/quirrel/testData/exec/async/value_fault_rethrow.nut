from "async" import Future
from "debug" import seterrorhandler

// A bare-value fault is caught after an await, then a NEW bare value is thrown
// from inside the catch. The rethrow runs under the generator-throw resume mode,
// so the origin-capture gate must snapshot the rethrow site (and drop the caught
// await's inherited trace) - the reported fault points at `reThrower`, carries
// the second value, and shows none of the first fault's frames.

seterrorhandler(function(err, trace) {
    assert(err == "second")          // the rethrown value, not the caught one
    assert(type(trace) == "array")
    local sawReThrower = false
    local sawInner = false
    foreach (fr in trace) {
        if (fr.func == "reThrower") sawReThrower = true
        if (fr.func == "inner") sawInner = true
    }
    assert(sawReThrower)                    // fresh origin captured at the rethrow
    assert(!sawInner)                       // the caught fault's origin is gone
    println("rethrow origin ok")
})

async function inner() {
    throw "first"
}

async function reThrower() {
    try {
        await inner()
        assert(false)
    } catch (e) {
        assert(e == "first")                // caught as the bare value
        throw "second"                       // new bare value from inside the catch
    }
}

reThrower()   // fire-and-forget
print("script done\n")
