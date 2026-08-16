options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE033: a run of >=2 bulk `push_from` / `push_clone_from` from arrays.
//
//   * into a FRESH-EMPTY array<T> (empty `var c : array<T>` then the run) it is a
//     concatenation — `var c <- concat(s1, s2, ...)` says it in one expression
//     (needs `require daslib/linq`). This is the N>=2 generalization of STYLE032
//     (which handles the single-source case with `var c := SRC`).
//   * into an EXISTING (already-live) array it collapses to a single variadic call
//     of the same name — `dst |> push_from(...)` or `dst |> push_clone_from(...)`
//     (the builtin fixed-arity overloads). This works for a plain variable receiver
//     (`dst`) AND a pure-chain receiver (`obj.arr`, `bags[0].nums`).
//
// Caps: concat tops out at 8 sources, the variadic push overloads at 4 — a run
// longer than its target shape can express stays silent (no invalid suggestion).
// A C-array (fixed-array) source has no clean array<T> rewrite, so it stays silent.
// A nested source (array<array<E>> into a flat array<E>) binds the recursive-flatten
// push_from overload — that is a flatten, not a clone/concat, so it stays silent too.
//
// `:=` / concat clone the source elements, so the FRESH-EMPTY suggestions are gated on
// the element being cloneable: a lambda array is copyable (`push_from` works) but NOT
// cloneable, so `var c := src` would not compile — STYLE032/033-concat stay silent for
// it. Existing-array variadic calls take const sources, so that form additionally
// requires elements that can be copied from const (a lambda existing-run still works).

expect 31209:7

require daslib/style_lint
require daslib/linq
require daslib/ast

struct Bag {
    nums : array<int>
}

typedef IntFn = lambda<(x : int) : int>

// --- Bad: fresh-empty array filled by a run (>=2) — concat ---

def bad_fresh_concat_clone(a, b : array<int>) : array<int> {
    var c : array<int>                        // STYLE033 — concat(a, b)
    c |> push_clone_from(a)
    c |> push_clone_from(b)
    return <- c
}

def bad_fresh_concat_copy(a, b, c : array<int>) : array<int> {
    var d : array<int>                        // STYLE033 — concat(a, b, c)
    d |> push_from(a)
    d |> push_from(b)
    d |> push_from(c)
    return <- d
}

// --- Bad: existing array (plain var) filled by a run (>=2) — variadic collapse ---

def bad_existing_variadic_clone(var dst : array<int>; a, b : array<int>) {
    dst |> push_clone_from(a)                 // STYLE033 — dst |> push_clone_from(a, b)
    dst |> push_clone_from(b)
}

def bad_existing_variadic_copy(var dst : array<int>; a, b : array<int>) {
    dst |> push_from(a)                       // STYLE033 — dst |> push_from(a, b)
    dst |> push_from(b)
}

// --- Bad: existing array via a pure CHAIN receiver — variadic collapse ---

def bad_chain_field_copy(var bag : Bag; a, b : array<int>) {
    bag.nums |> push_from(a)                  // STYLE033 — bag.nums |> push_from(a, b)
    bag.nums |> push_from(b)
}

def bad_chain_indexed_clone(var bags : array<Bag>; a, b : array<int>) {
    bags[0].nums |> push_clone_from(a)        // STYLE033 — bags[0].nums |> push_clone_from(a, b)
    bags[0].nums |> push_clone_from(b)
}

// --- Bad: lambda array is copyable, so the EXISTING variadic collapse still applies ---

def bad_lambda_existing_variadic(var dst : array<IntFn>; a, b : array<IntFn>) {
    dst |> push_from(a)                       // STYLE033 — dst |> push_from(a, b) (copy, no clone)
    dst |> push_from(b)
}

// --- Good: already the suggested forms / out of scope (must NOT fire) ---

def good_concat_form(a, b : array<int>) : array<int> {
    // Already a concat.
    return <- concat(a, b)
}

def good_already_variadic(a, b : array<int>) : array<int> {
    // Already collapsed to one variadic call.
    var c : array<int>
    c |> push_clone_from(a, b)
    return <- c
}

def good_c_array_run() : array<int> {
    // C-array sources — no clean array<T> concat rewrite, stays silent.
    let c1 = fixed_array(1, 2)
    let c2 = fixed_array(3, 4)
    var w : array<int>
    w |> push_from(c1)
    w |> push_from(c2)
    return <- w
}

def good_existing_too_long(var dst : array<int>; a, b, c, d, e : array<int>) {
    // A run of 5 has no single variadic overload (cap is 4) — stays silent.
    dst |> push_clone_from(a)
    dst |> push_clone_from(b)
    dst |> push_clone_from(c)
    dst |> push_clone_from(d)
    dst |> push_clone_from(e)
}

def good_fresh_flatten(n1, n2 : array<array<int>>) : array<int> {
    // Nested sources flatten via the recursive push_from overload — NOT a clone, so
    // `concat(n1, n2)` (array<array<int>>) would be the wrong type. Must stay silent.
    var w : array<int>
    w |> push_from(n1)
    w |> push_from(n2)
    return <- w
}

def good_single_flatten(n1 : array<array<int>>) : array<int> {
    // Single nested source flattens — STYLE032's `var w := n1` would be a type error.
    var w : array<int>
    w |> push_from(n1)
    return <- w
}

def good_existing_flatten(var dst : array<int>; n1, n2 : array<array<int>>) {
    // Nested sources flatten into a flat dst — not a flat-array bulk append; stays silent.
    dst |> push_from(n1)
    dst |> push_from(n2)
}

// --- Good: chain receiver, out of scope (must NOT fire) ---

def good_chain_single(var bag : Bag; a : array<int>) {
    // A single chain push is not a run — no variadic collapse to suggest.
    bag.nums |> push_from(a)
}

def good_chain_mixed_fname(var bag : Bag; a, b : array<int>) {
    // push_from then push_clone_from cannot merge (different op) — each sub-run is 1.
    bag.nums |> push_from(a)
    bag.nums |> push_clone_from(b)
}

def good_chain_flatten(var bag : Bag; n1, n2 : array<array<int>>) {
    // Nested sources flatten into the flat chain field — not a bulk append; stays silent.
    bag.nums |> push_from(n1)
    bag.nums |> push_from(n2)
}

// --- Good: lambda array fresh-empty — copyable but not cloneable, gate suppresses ---

def good_lambda_fresh_single(src : array<IntFn>) : array<IntFn> {
    // `var c := src` would not compile (lambda not cloneable) — STYLE032 must stay silent.
    var c : array<IntFn>
    c |> push_from(src)
    return <- c
}

def good_lambda_fresh_run(a, b : array<IntFn>) : array<IntFn> {
    // `concat(a, b)` clones — not valid for a lambda array. STYLE033-concat must stay silent.
    var c : array<IntFn>
    c |> push_from(a)
    c |> push_from(b)
    return <- c
}

def good_mutable_only_existing(var dst : array<Expression?>; var a, b : array<Expression?>) {
    // Expression smart pointers copy from mutable arrays, but not from the const sources
    // required by the variadic overload.
    dst |> push_from(a)
    dst |> push_from(b)
}
