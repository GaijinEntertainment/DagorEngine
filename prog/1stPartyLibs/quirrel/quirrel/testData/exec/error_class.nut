// Verifies the built-in Error class.

// 1. basic construction
let e = Error("oops")
assert(e.message == "oops")
assert(e.trace == null)
assert(e.cause == null)
println(e.message)

// 2. no-args -- defaults
let e0 = Error()
assert(e0.message == "")
assert(e0.trace == null)
assert(e0.cause == null)

// 3. two-arg: cause is set
let e1 = Error("a", "b")
assert(e1.message == "a")
assert(e1.cause == "b")

// 4. _tostring returns .message
println(Error("hello"))

// 5. string concat coerces via _tostring
println("got: " + Error("x"))

// 6. instanceof Error
assert(Error("x") instanceof Error)

// 7. Python-style subclassing -- inherits slots and _tostring
class E2(Error) {}
let e2 = E2()
assert(e2.message == "")
assert(e2 instanceof Error)
assert(e2 instanceof E2)

// 8. subclass with constructor chain
class E3(Error) {
    constructor() { base.constructor("e3msg") }
}
let e3 = E3()
assert(e3.message == "e3msg")
assert(e3 instanceof Error)

// 9. trace slot is writable
let etrace = Error("t")
etrace.trace = "stub"
assert(etrace.trace == "stub")

// 10. cause is arbitrary -- not constrained to Error
let earb = Error("x", 42)
assert(earb.cause == 42)

// 11. cause can be another Error (chain shape)
let einner = Error("inner")
let eouter = Error("outer", einner)
assert(eouter.cause == einner)
assert(eouter.cause.message == "inner")

// 12. non-string message is rejected by the constructor's parameter typecheck
local rejected = false
try {
    Error(42)
} catch (e) {
    rejected = true
}
assert(rejected)

// 13. throw/catch round-trip preserves the Error instance
try {
    throw Error("boom")
} catch (e) {
    assert(e instanceof Error)
    assert(e.message == "boom")
}

// 14. sticky rethrow keeps the same instance (fields preserved)
let original = Error("orig")
original.trace = "stub-trace"
try {
    try {
        throw original
    } catch (e) {
        throw e
    }
} catch (e) {
    assert(e instanceof Error)
    assert(e.message == "orig")
    assert(e.trace == "stub-trace")
}

// 15. catch handler receives the subclass instance, not the base
class E15(Error) {
    code = null
    constructor(m, c) { base.constructor(m); this.code = c }
}
try {
    throw E15("e15msg", 7)
} catch (e) {
    assert(e instanceof E15)
    assert(e instanceof Error)
    assert(e.message == "e15msg")
    assert(e.code == 7)
}

// 16. cause chaining (rewrap pattern) keeps the inner error reachable
try {
    try {
        throw Error("inner")
    } catch (e) {
        throw Error("outer", e)
    }
} catch (e) {
    assert(e.message == "outer")
    assert(e.cause instanceof Error)
    assert(e.cause.message == "inner")
}

println("all ok")
