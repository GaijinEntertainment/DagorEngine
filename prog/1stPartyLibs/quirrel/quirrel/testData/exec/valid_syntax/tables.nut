// Valid syntax: Tables
#allow-delete-operator

// Empty table
let empty = {}
assert(empty.len() == 0)

// Table with identifier keys
let t1 = {
    name = "John",
    age = 30,
    active = true
}
assert(t1.name == "John")
assert(t1.age == 30)
assert(t1.active == true)

// Table with computed/bracket keys
let t2 = {
    ["dynamic-key"] = "value",
    [42] = "numeric key",
    ["a" + "b"] = "computed"
}
assert(t2["dynamic-key"] == "value")
assert(t2[42] == "numeric key")
assert(t2["ab"] == "computed")

// Table with methods
let t3 = {
    value = 10,
    function get() { return this.value },
    function set(v) { this.value = v }
}
assert(t3.get() == 10)
t3.set(20)
assert(t3.get() == 20)

// ES2015 shorthand
local x = 123
local y = 456
let shorthand = {x, y}
assert(shorthand.x == 123)
assert(shorthand.y == 456)

// Mixed shorthand and explicit
local z = 789
let mixed = {x, y = 20, z}
assert(mixed.x == 123)
assert(mixed.y == 20)
assert(mixed.z == 789)

// Trailing comma
let trailing = {
    a = 1,
    b = 2,
}
assert(trailing.a == 1)

// No comma (space-separated)
let no_comma = {
    a = 1
    b = 2
    c = 3
}
assert(no_comma.len() == 3)

// Nested table
let nested = {
    inner = {
        deep = {
            value = 42
        }
    }
}
assert(nested.inner.deep.value == 42)

// Slot creation with <-
local t4 = {}
t4.newkey <- "hello"
assert(t4.newkey == "hello")
t4["another"] <- 42
assert(t4.another == 42)

// Slot deletion
local t5 = {a = 1, b = 2, c = 3}
delete t5.a
assert(!("a" in t5))
assert(t5.len() == 2)

// Table methods
let methods_tbl = {a = 1, b = 2, c = 3}
assert(methods_tbl.len() == 3)
assert(methods_tbl.rawin("a"))
assert(methods_tbl.rawget("a") == 1)

let keys = methods_tbl.keys()
assert(keys.len() == 3)

let values = methods_tbl.values()
assert(values.len() == 3)

// Table functional methods
let mapped = methods_tbl.map(@(v) v * 2)
assert(mapped.a == 2)

let filtered = methods_tbl.filter(@(v) v > 1)
assert(!("a" in filtered))
assert("b" in filtered)

local each_sum = 0
methods_tbl.each(@(v) each_sum += v)
assert(each_sum == 6)

// rawset returns the table (for chaining)
let chain_tbl = {}
chain_tbl.rawset("a", 1).rawset("b", 2).rawset("c", 3)
assert(chain_tbl.a == 1)
assert(chain_tbl.b == 2)
assert(chain_tbl.c == 3)

// topairs
let pairs = {x = 10, y = 20}.topairs()
assert(pairs.len() == 2)

// __merge
let base_tbl = {a = 1, b = 2}
let extra = {b = 3, c = 4}
let merged = base_tbl.__merge(extra)
assert(merged.a == 1)
assert(merged.b == 3)
assert(merged.c == 4)
assert(base_tbl.b == 2) // original unchanged

// __update (in-place)
local updatable = {a = 1, b = 2}
updatable.__update({b = 3, c = 4})
assert(updatable.b == 3)
assert(updatable.c == 4)

// in / not in
let membership = {a = 1, b = 2}
assert("a" in membership)
assert("c" not in membership)

// Iteration
local iter_count = 0
foreach (k, v in {a = 1, b = 2, c = 3}) {
    iter_count++
}
assert(iter_count == 3)

// clone table
let orig = {x = 1, y = [1, 2]}
let cloned = clone orig
cloned.x = 99
assert(orig.x == 1) // original not modified

// Table with function values
let fn_tbl = {
    add = @(a, b) a + b,
    mul = @(a, b) a * b,
}
assert(fn_tbl.add(2, 3) == 5)
assert(fn_tbl.mul(2, 3) == 6)

print("tables: OK\n")
