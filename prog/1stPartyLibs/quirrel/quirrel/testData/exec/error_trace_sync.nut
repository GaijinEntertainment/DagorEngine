// Verifies that a thrown Error captures an origin stack trace into e.trace.

function inner() {
    throw Error("boom")
}
function outer() {
    inner()
}

// 1. a caught Error carries a trace: an array of { func, source, line } frames,
//    innermost first.
try {
    outer()
} catch (e) {
    assert(e instanceof Error)
    assert(type(e.trace) == "array")
    assert(e.trace.len() >= 2)

    let top = e.trace[0]
    assert(top.func == "inner")
    assert(top.line == 4)

    let next = e.trace[1]
    assert(next.func == "outer")
    assert(next.line == 7)
}

// 2. an Error constructed but never thrown has no trace.
let unthrown = Error("x")
assert(unthrown.trace == null)

// 3. a pre-set trace is preserved (sticky rethrow): capture only fills a null slot.
let preset = Error("y")
preset.trace = "stub"
try {
    throw preset
} catch (e) {
    assert(e.trace == "stub")
}

// 4. a non-Error throw is unaffected (no trace machinery).
try {
    throw "plain string"
} catch (e) {
    assert(type(e) == "string")
}

println("all ok")
