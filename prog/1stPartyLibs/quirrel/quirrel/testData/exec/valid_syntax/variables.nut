// Valid syntax: Variable declarations and scoping

// local declaration
local x = 10
local y
assert(x == 10)
assert(y == null)

// Multiple local declarations
local a = 1, b = 2, c = 3
assert(a == 1)
assert(b == 2)
assert(c == 3)

// let declarations (immutable bindings)
let lx = 42
let ly = "hello"
assert(lx == 42)
assert(ly == "hello")

// let with mutable objects
let arr = [1, 2, 3]
arr[0] = 99
assert(arr[0] == 99)

let tbl = {a = 1}
tbl.a = 2
assert(tbl.a == 2)

// Block scoping
{
    local inner = 100
    assert(inner == 100)
}

// local in for loop
for (local i = 0; i < 3; i++) {
    assert(i >= 0)
}

// Multiple assignment
local m1 = 1
local m2 = 2
m1 = 10
m2 = 20
assert(m1 == 10)
assert(m2 == 20)

// Variable referencing earlier variable
local va = 5
local vb = va + 1
assert(vb == 6)

// Uninitialized local defaults to null
local uninit
assert(uninit == null)

// Nested scopes with different variable names
local outer = "outer"
{
    local inner2 = "inner"
    assert(inner2 == "inner")
    assert(outer == "outer")
}
assert(outer == "outer")

print("variables: OK\n")
