// Test: file-level 'let' variables holding mutable values must NOT cause
// auto-static-memoization when passed to [pure] functions.

let config = { value = 10 }

// --- pure function returning a primitive derived from mutable state ---

function [pure] readValue(t) {
    return t.value
}

function getFromConfig() {
    return readValue(config)
}

assert(getFromConfig() == 10)
config.value = 42
assert(getFromConfig() == 42)
println("primitive return: ok")


// --- pure function creating a new table from mutable state ---

config.value = 100

function [pure] snapshot(t) {
    return { captured = t.value }
}

function takeSnapshot() {
    return snapshot(config)
}

assert(takeSnapshot().captured == 100)
config.value = 200
assert(takeSnapshot().captured == 200)
println("new table return: ok")


// --- pure function returning its argument (must not freeze it) ---

let data = { x = 1 }

function [pure] passthrough(t) {
    return t
}

function getData() {
    return passthrough(data)
}

let d = getData()
d.x = 999
assert(d.x == 999)
assert(data.x == 999)
println("passthrough not frozen: ok")


// --- multiple mutable arguments ---

let left  = { n = 1 }
let right = { n = 2 }

function [pure] addFields(a, b) {
    return a.n + b.n
}

function computeSum() {
    return addFields(left, right)
}

assert(computeSum() == 3)
left.n = 10
right.n = 20
assert(computeSum() == 30)
println("two mutable args: ok")


// --- nested pure calls ---

config.value = 5

function [pure] double(n) {
    return n * 2
}

function getDoubled() {
    return double(readValue(config))
}

assert(getDoubled() == 10)
config.value = 50
assert(getDoubled() == 100)
println("nested pure calls: ok")
