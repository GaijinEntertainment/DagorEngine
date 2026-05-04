// Valid syntax: Type annotations

// Variable type annotations
local xi: int = 42
local xf: float = 3.14
local xs: string = "hello"
local xb: bool = true
local xn: null = null
local xa: array = [1, 2]
local xt: table = {a = 1}
local xfn: function = @(x) x

// let with type annotations
let li: int = 100
let lf: float = 2.71
let ls: string = "world"

// Union types
local nu: int|null = null
nu = 42
local ns: string|null = "text"
ns = null
local nif: int|float = 42
nif = 3.14

// Function parameter types
function typed_add(a: int, b: int): int {
    return a + b
}
assert(typed_add(2, 3) == 5)

// Function with union param types
function flex(val: int|string) {
    return val
}
assert(flex(42) == 42)
assert(flex("hi") == "hi")

// Function with nullable params
function opt(x: int|null = null): int {
    return x ?? 0
}
assert(opt() == 0)
assert(opt(10) == 10)

// Lambda with type annotations
let typed_lambda = @(x: int, y: int): int x + y
assert(typed_lambda(3, 4) == 7)

// number type (int | float)
function accept_number(n: number): number {
    return n
}
assert(accept_number(42) == 42)
assert(accept_number(3.14) == 3.14)

// any type
function accept_any(v: any): any {
    return v
}
assert(accept_any(42) == 42)
assert(accept_any("str") == "str")
assert(accept_any(null) == null)

// Varargs type annotation
function sum_floats(first: float, ...: float): float {
    local total = first
    foreach (v in vargv) total += v
    return total
}
assert(sum_floats(1.0, 2.0, 3.0) == 6.0)

// Complex return type
function maybe_string(flag: bool): string|null {
    if (flag) return "yes"
    return null
}
assert(maybe_string(true) == "yes")
assert(maybe_string(false) == null)

print("type_annotations: OK\n")
