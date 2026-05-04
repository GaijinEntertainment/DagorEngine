// Tutorial 19: Generic Programming
//
// This tutorial covers:
//   - Generic functions (omitting types / using auto)
//   - Named auto for type constraints (auto(T))
//   - Using auto(T) for local variables
//   - default<T> for default values
//   - Specialization: from generic to specific
//   - typeinfo for compile-time type inspection
//   - static_if for conditional compilation
//   - has_field and struct introspection
//   - Practical generic patterns
//
// Run: daslang.exe tutorials/language/19_generics.das

options gen2

// === Generic function: auto-inferred types ===
// Omitting a type makes a function generic. It is instantiated
// for each concrete type on first use.

def print_value(v) {
    print("  value = {v}\n")
}

// === Named auto: ensure types match ===
// auto(T) names the type so multiple arguments share the same type.

def add(a, b : auto(T)) : T {
    return a + b
}

// === auto(T) for local variables ===
// Use the named type T in local variable declarations.

def make_pair(a, b : auto(T)) {
    var first : T = a
    var second : T = b
    print("  pair: {first}, {second}\n")
}

// === Specialization: from generic to specific ===
// The compiler picks the most specific matching overload.
// Order: specific type > constrained generic > unconstrained generic

// Most generic: takes anything
def process(v) {
    print("  anything: {v}\n")
}

// More specific: any array (T is the element type)
def process(arr : array<auto(T)>) {
    print("  any array<{typeinfo typename(type<T>)}>, length={length(arr)}\n")
}

// Most specific: array<int> (exact match)
def process(arr : array<int>) {
    var total = 0
    for (v in arr) {
        total += v
    }
    print("  array<int>, sum={total}\n")
}

// === typeinfo: compile-time inspection ===
// Use typeinfo to query the type of an argument.

def describe_type(v : auto(T)) {
    let name = typeinfo typename(type<T>)
    let size = typeinfo sizeof(type<T>)
    print("  type={name}, size={size}\n")
}

// === static_if: conditional compilation ===
// Different code paths based on type properties.

def to_string_generic(v : auto(T)) : string {
    static_if (typeinfo is_numeric(type<T>)) {
        return "number: {v}"
    } else {
        return "other: {v}"
    }
}

// === has_field: struct introspection ===
// Check if an expression has a particular field.

struct Player {
    name : string
    health : int
}

struct Item {
    name : string
    weight : int
}

struct Nameless {
    id : int
}

def get_name(obj) : string {
    static_if (typeinfo has_field<name>(obj)) {
        return obj.name
    } else {
        return "unnamed"
    }
}

// === Typed array generics ===
// Constrain a generic to accept only arrays.

def sum_array(arr : array<auto(T)>) : T {
    var total = default<T>    // default value for any type (0, 0.0, "", etc.)
    for (v in arr) {
        total += v
    }
    return total
}

def first_or_default(arr : array<auto(T)>; default_val : T) : T {
    if (length(arr) > 0) {
        return arr[0]
    }
    return default_val
}

[export]
def main {

    // === Generic function with auto ===
    print("print_value:\n")
    print_value(42)
    print_value(3.14)
    print_value("hello")

    // === Named auto: same type for both args ===
    print("add:\n")
    print("  int: {add(10, 20)}\n")
    print("  float: {add(1.5, 2.5)}\n")
    // add(1, 2.0) would fail: int and float don't match

    // === auto(T) for local variables ===
    print("make_pair:\n")
    make_pair(10, 20)
    make_pair("hello", "world")

    // === Specialization ===
    // The compiler picks the most specific overload:
    print("specialization:\n")
    process(42)                       // → anything (plain int)

    var strs <- ["a", "b", "c"]
    process(strs)                     // → any array (array<string>)

    var ints <- [1, 2, 3]
    process(ints)                     // → array<int> (exact match)

    // === typeinfo ===
    print("describe_type:\n")
    describe_type(42)
    describe_type(3.14)

    // === static_if ===
    print("to_string_generic:\n")
    print("  {to_string_generic(99)}\n")
    print("  {to_string_generic("hello")}\n")

    // === has_field introspection ===
    print("get_name:\n")
    let p = Player(name = "Hero", health = 100)
    let item = Item(name = "Sword", weight = 5)
    let thing = Nameless(id = 7)
    print("  player: {get_name(p)}\n")
    print("  item:   {get_name(item)}\n")
    print("  nameless: {get_name(thing)}\n")

    // === Array generics ===
    print("sum_array:\n")
    var int_arr <- [1, 2, 3, 4, 5]
    var float_arr <- [1.0, 2.0, 3.0]
    print("  ints: {sum_array(int_arr)}\n")
    print("  floats: {sum_array(float_arr)}\n")

    print("first_or_default:\n")
    var names <- ["Alice", "Bob"]
    var empty : array<string>
    print("  names: {first_or_default(names, "nobody")}\n")
    print("  empty: {first_or_default(empty, "nobody")}\n")

    // === Summary ===
    // - Omit types or use 'auto' to make functions generic
    // - auto(T) names a type variable — use it for args AND local vars
    // - Specialization: more specific overloads win
    //     def process(v)              — takes anything
    //     def process(arr:array<auto>) — takes any array
    //     def process(arr:array<int>)  — takes array<int>
    // - typeinfo queries types at compile time (typename, sizeof, is_*)
    // - static_if selects code paths based on type traits
    // - default<T> gives the default value of any type (0, 0.0, "", etc.)
    // - has_field checks for struct fields generically
    // - Generic functions are instantiated per call site
}

// output:
// print_value:
//   value = 42
//   value = 3.14
//   value = hello
// add:
//   int: 30
//   float: 4
// make_pair:
//   pair: 10, 20
//   pair: hello, world
// specialization:
//   anything: 42
//   any array<string>, length=3
//   array<int>, sum=6
// describe_type:
//   type=int const, size=4
//   type=float const, size=4
// to_string_generic:
//   number: 99
//   other: hello
// get_name:
//   player: Hero
//   item:   Sword
//   nameless: unnamed
// sum_array:
//   ints: 15
//   floats: 6
// first_or_default:
//   names: Alice
//   empty: nobody
