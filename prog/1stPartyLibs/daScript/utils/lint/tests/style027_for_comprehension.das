options gen2
// STYLE027: var array<T> / table<K;V> with empty default-init followed by a
// for-loop that only push/insert into it — use a comprehension.
//
// Problem: Declaring an empty `array<T>` (or `table<K;V>`) and then populating
// it with a for-loop whose body is solely `a |> push(...)` / `a |> insert(k,v)`
// / `a[k] = v` (optionally wrapped in `if (cond) { ... }`) is the loop variant
// of STYLE012. A comprehension expresses the same transformation in one fused
// expression — no intermediate iterator, no append cost — and reads better.
//
// Bad pattern:
//   var a : array<int>
//   for (x in SRC) {
//       if (x < 19) {
//           a |> push(x*x)
//       }
//   }
//
// Good pattern:
//   var a <- [for (x in SRC); x*x; where x < 19]
//
// Match rules:
//   * outer ExprLet declares the array/table local with NO initializer
//   * the very next statement in the enclosing block is the ExprFor
//   * the for body (recursively, with at most one nested ExprFor — depth ≤ 2)
//     contains ONLY: push/push_clone (array form), insert / `a[k] = v` (table
//     form), all targeting the freshly-declared local. `if (cond) { ... }`
//     wrappers are accepted at any depth and lift to the comprehension's
//     single `where` clause.
//   * `emplace` is intentionally excluded — different lifetime semantics.

expect 31209:8

require daslib/style_lint

struct Foo {
    v : int = 0
}

// --- Bad patterns (must fire — 8 total) ---

def bad_single_source_no_filter(src : array<int>) : int {
    var a : array<int>                                // STYLE027 — comprehension
    for (x in src) {
        a |> push(x * x)
    }
    return length(a)
}

def bad_single_source_with_filter(src : array<int>) : int {
    var a : array<int>                                // STYLE027 — comprehension + where
    for (x in src) {
        if (x < 19) {
            a |> push(x * x)
        }
    }
    return length(a)
}

def bad_push_clone(src : array<string>) : int {
    var a : array<string>                             // STYLE027 — push_clone form
    for (s in src) {
        a |> push_clone(s)
    }
    return length(a)
}

def bad_nested_2_level(src1, src2 : array<int>) : int {
    var a : array<int>                                // STYLE027 — nested-for, two sources
    for (x in src1) {
        for (y in src2) {
            a |> push(x + y)
        }
    }
    return length(a)
}

def bad_multi_source_single_for(tab : table<string; int>) : int {
    var a : array<string>                             // STYLE027 — multi-source single for
    for (k, v in keys(tab), values(tab)) {
        a |> push("{k}={v}")
    }
    return length(a)
}

def bad_outer_if_filter(src1, src2 : array<int>) : int {
    var a : array<int>                                // STYLE027 — if wrapping a nested for
    for (x in src1) {
        if (x > 0) {
            for (y in src2) {
                a |> push(x + y)
            }
        }
    }
    return length(a)
}

def bad_table_insert(src : array<int>) : int {
    var t : table<int; string>                        // STYLE027 — table form via insert
    for (x in src) {
        t |> insert(x, "v{x}")
    }
    return length(t)
}

def bad_table_at_assign(src : array<int>) : int {
    var t : table<int; int>                           // STYLE027 — table form via t[k] = v
    for (x in src) {
        t[x] = x * 2
    }
    return length(t)
}

// --- Good patterns (must NOT fire) ---

def good_preinitialized(src : array<int>) : int {
    // Init is non-empty — comprehension rewrite would lose [1,2,3]; skip.
    var a <- [1, 2, 3]
    for (x in src) {
        a |> push(x)
    }
    return length(a)
}

def good_intervening_statement(src : array<int>) : int {
    // Decl and for-loop are not adjacent — STYLE027 requires immediate-next.
    var a : array<int>
    let n = 42
    for (x in src) {
        a |> push(x + n)
    }
    return length(a)
}

def good_two_statements_in_body(src : array<int>) : int {
    // Body has 2 stmts — body must be exactly one push/insert (or if-wrap).
    var a : array<int>
    var total = 0
    for (x in src) {
        a |> push(x)
        total += x
    }
    return total + length(a)
}

def good_emplace(src : array<int>) : int {
    // emplace is explicitly out of scope (move zeros source — different semantics).
    var a : array<Foo>
    for (x in src) {
        a |> emplace(Foo(v = x))
    }
    return length(a)
}

def good_depth3_nested(src1, src2, src3 : array<int>) : int {
    // Three nested fors — beyond the depth-2 budget.
    var a : array<int>
    for (x in src1) {
        for (y in src2) {
            for (z in src3) {
                a |> push(x + y + z)
            }
        }
    }
    return length(a)
}

def good_push_to_different_var(src : array<int>) : int {
    // The immediate-prev let (`a`) is empty; for-loop pushes into `b`, which
    // was declared earlier — not the immediately-prev empty let.
    var b : array<int>
    var a : array<int>
    for (x in src) {
        b |> push(x)
    }
    return length(a) + length(b)
}

def good_uses_comprehension(src : array<int>) : int {
    // The idiomatic form — must not trigger on its own AST (lowered comprehension
    // carries genFlags.generated, and the synthesized ExprFor lives inside a
    // closure, not in our enclosing block).
    var a <- [for (x in src); x * x; where x > 0]
    return length(a)
}
