// Valid syntax: Generators and yield

// Basic generator
function count_to(n) {
    for (local i = 1; i <= n; i++)
        yield i
}

// Using resume
let gen = count_to(3)
assert(resume gen == 1)
assert(resume gen == 2)
assert(resume gen == 3)
assert(resume gen == null)

// Generator with foreach
local sum = 0
foreach (v in count_to(5)) {
    sum += v
}
assert(sum == 15) // 1+2+3+4+5

// Fibonacci generator
function fib_gen(n) {
    local a = 0, b = 1
    for (local i = 0; i < n; i++) {
        yield a
        let temp = a + b
        a = b
        b = temp
    }
}

let fibs = []
foreach (f in fib_gen(8)) {
    fibs.append(f)
}
assert(fibs[0] == 0)
assert(fibs[1] == 1)
assert(fibs[7] == 13)

// Generator yielding different types
function mixed_gen() {
    yield 42
    yield "hello"
    yield [1, 2, 3]
    yield {a = 1}
}

let mg = mixed_gen()
assert(resume mg == 42)
assert(resume mg == "hello")
let arr = resume mg
assert(arr.len() == 3)
let tbl = resume mg
assert(tbl.a == 1)

// Empty generator (no yields)
function empty_gen() {
    return null
}

let eg = empty_gen()
// Not really a generator, just a function

// Generator with yield and no value
function void_gen(n) {
    for (local i = 0; i < n; i++)
        yield
}

let vg = void_gen(3)
assert(resume vg == null)
assert(resume vg == null)
assert(resume vg == null)

// Infinite generator (controlled with break)
function naturals() {
    local n = 1
    while (true) {
        yield n
        n++
    }
}

let nat = naturals()
local first_five = []
for (local i = 0; i < 5; i++) {
    first_five.append(resume nat)
}
assert(first_five[0] == 1)
assert(first_five[4] == 5)

// Range generator
function range(start, stop, step = 1) {
    for (local i = start; i < stop; i += step)
        yield i
}

local range_vals = []
foreach (v in range(0, 10, 2)) {
    range_vals.append(v)
}
assert(range_vals.len() == 5)
assert(range_vals[0] == 0)
assert(range_vals[4] == 8)

print("generators: OK\n")
