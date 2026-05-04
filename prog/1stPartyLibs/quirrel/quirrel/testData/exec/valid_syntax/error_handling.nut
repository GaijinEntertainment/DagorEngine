// Valid syntax: Error handling (try/catch/throw)

// Basic try/catch
local caught = null
try {
    throw "test error"
} catch (e) {
    caught = e
}
assert(caught == "test error")

// Catching runtime errors
local rt_caught = false
try {
    let t = {}
    let _ = t.nonexistent_method()
} catch (e) {
    rt_caught = true
}
assert(rt_caught)

// Throw integer
local int_err = null
try {
    throw 42
} catch (e) {
    int_err = e
}
assert(int_err == 42)

// Throw null
local null_err = "not null"
try {
    throw null
} catch (e) {
    null_err = e
}
assert(null_err == null)

// Throw table
local tbl_err = null
try {
    throw {code = 404, message = "Not found"}
} catch (e) {
    tbl_err = e
}
assert(tbl_err.code == 404)

// Nested try/catch
local outer_caught = null
local inner_caught = null
try {
    try {
        throw "inner"
    } catch (e) {
        inner_caught = e
        throw "outer"
    }
} catch (e) {
    outer_caught = e
}
assert(inner_caught == "inner")
assert(outer_caught == "outer")

// Try/catch with no exception
local no_err = true
try {
    let x = 42
} catch (e) {
    no_err = false
}
assert(no_err)

// Function that may throw
function divide(a, b) {
    if (b == 0) throw "division by zero"
    return a / b
}

assert(divide(10, 2) == 5)

local div_err = null
try {
    divide(10, 0)
} catch (e) {
    div_err = e
}
assert(div_err == "division by zero")

// Try/catch in loop
local errors_caught = 0
for (local i = 0; i < 5; i++) {
    try {
        if (i % 2 == 0) throw "even error"
    } catch (e) {
        errors_caught++
    }
}
assert(errors_caught == 3) // 0, 2, 4

// Rethrow
local rethrown = null
try {
    try {
        throw "original"
    } catch (e) {
        throw e // rethrow
    }
} catch (e) {
    rethrown = e
}
assert(rethrown == "original")

print("error_handling: OK\n")
