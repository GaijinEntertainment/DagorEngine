// Valid syntax: Null safety operators

// Null-conditional member access
let obj = {name = "test", nested = {value = 42}}
assert(obj?.name == "test")
assert(obj?.nested?.value == 42)

let null_obj = null
assert(null_obj?.name == null)
assert(null_obj?.nested?.value == null)

// Null-conditional indexing
let arr = [1, 2, 3]
assert(arr?[0] == 1)
assert(arr?[2] == 3)

let null_arr = null
assert(null_arr?[0] == null)

// Null-conditional with table bracket
let tbl = {a = 1, b = 2}
assert(tbl?["a"] == 1)
let null_tbl = null
assert(null_tbl?["a"] == null)

// Null-coalescing operator
assert((null ?? "default") == "default")
assert(("value" ?? "default") == "value")
assert((0 ?? "default") == 0) // 0 is not null
assert((false ?? "default") == false) // false is not null
assert(("" ?? "default") == "")  // empty string is not null

// Chained null-coalescing
assert((null ?? null ?? "last") == "last")
assert((null ?? "middle" ?? "last") == "middle")

// Null-safe function call
function greet(name) { return "Hello " + name }
let fn = greet
assert(fn?("World") == "Hello World")

let null_fn = null
assert(null_fn?("World") == null)

// Null propagation through chain
let deep = {level1 = {level2 = {level3 = "found"}}}
assert(deep?.level1?.level2?.level3 == "found")
assert(deep?.missing?.level2?.level3 == null)

// Null-safe type method access
let s = "hello"
assert(s?.$len() == 5)
let null_s = null
assert(null_s?.$len() == null)

// Practical patterns
function safe_get(tbl, key, default_val = null) {
    return tbl?[key] ?? default_val
}
assert(safe_get({a = 1}, "a") == 1)
assert(safe_get({a = 1}, "b", 0) == 0)
assert(safe_get(null, "a", 0) == 0)

// Null-safe in conditional
let maybe_obj = null
if (maybe_obj?.value != null) {
    assert(false) // should not reach
}

let real_obj = {value = 42}
if (real_obj?.value != null) {
    assert(real_obj.value == 42)
}

print("null_safety: OK\n")
