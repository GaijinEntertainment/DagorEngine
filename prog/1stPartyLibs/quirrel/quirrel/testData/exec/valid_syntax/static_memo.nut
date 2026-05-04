// Valid syntax: Static memoization

// Static with simple value
function get_config() {
    return static {
        name = "app",
        version = "1.0"
    }
}

let cfg1 = get_config()
let cfg2 = get_config()
// Both return the same cached frozen object

assert(cfg1.name == "app")
assert(cfg1.version == "1.0")

// Static with array
function get_list() {
    return static [1, 2, 3, 4, 5]
}
let list1 = get_list()
assert(list1.len() == 5)
assert(list1[0] == 1)

// Static with simple expression
function get_value() {
    return static 42 * 2 + 1
}
assert(get_value() == 85)

// Static objects are frozen
let frozen_tbl = static {a = 1, b = 2}
assert(frozen_tbl.is_frozen())

let frozen_arr = static [1, 2, 3]
assert(frozen_arr.is_frozen())

// Inline const expressions
let const_arr = const [10, 20, 30]
assert(const_arr.is_frozen())
assert(const_arr[0] == 10)

let const_tbl = const {x = 1, y = 2}
assert(const_tbl.is_frozen())

// Const function
let const_fn = const @(x) x * 2
assert(const_fn(5) == 10)

// freeze function
let mutable_arr = [1, 2, 3]
let immutable_arr = freeze(mutable_arr)
assert(immutable_arr.is_frozen())

let mutable_tbl = {a = 1}
let immutable_tbl = freeze(mutable_tbl)
assert(immutable_tbl.is_frozen())

print("static_memo: OK\n")
