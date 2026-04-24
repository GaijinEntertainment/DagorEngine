options gen2
options no_aot

// Tutorial 16 — Template Type Macro (structure generation)
//
// This tutorial uses the `pair` type macro from template_type_macro_mod
// to create parameterized structures.  `pair(type<T1>, type<T2>)` expands
// to a struct with `first : T1` and `second : T2`.
//
// Three patterns are demonstrated:
//   1. Concrete instantiation — fully specified types
//   2. Generic parameter     — auto-deduced type arguments
//   3. Generic return type   — the macro in return-type position

require template_type_macro_mod

// --- 1. Concrete instantiation ---
// `pair(type<int>, type<float>)` generates a struct `Pair<int,float>`
// with `first : int` and `second : float`.

def test_concrete() {
    var p : pair(type<int>, type<float>)
    p.first = 42
    p.second = 3.14
    print("concrete: first={p.first} second={p.second}\n")
}

// --- 2. Generic parameter ---
// The function accepts any Pair instantiation.  The compiler matches
// `auto(T1)` / `auto(T2)` against the stored structure aliases.

def get_first(p : pair(type<auto(T1)>, type<auto(T2)>)) : T1 {
    return p.first
}

def get_second(p : pair(type<auto(T1)>, type<auto(T2)>)) : T2 {
    return p.second
}

// --- 3. Generic return type ---
// The macro can appear in return-type position as well.  Here we swap
// the type arguments — `pair(type<T2>, type<T1>)` — so the returned
// struct has the types reversed.

def swap_pair(p : pair(type<auto(T1)>, type<auto(T2)>)) : pair(type<T2>, type<T1>) {
    var result : pair(type<T2>, type<T1>)
    result.first = p.second
    result.second = p.first
    return result
}

[export]
def main() {
    test_concrete()

    // Generic parameter: get_first / get_second.
    var p : pair(type<int>, type<float>)
    p.first = 10
    p.second = 2.5
    print("get_first:  {get_first(p)}\n")
    print("get_second: {get_second(p)}\n")

    // Generic return type: swap_pair.
    var swapped = swap_pair(p)
    print("swapped: first={swapped.first} second={swapped.second}\n")
}

// Expected output:
//   concrete: first=42 second=3.14
//   get_first:  10
//   get_second: 2.5
//   swapped: first=2.5 second=10
