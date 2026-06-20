options gen2
// STYLE013: struct (or `new`-allocated struct pointer) var with default/empty
// init followed by a run of field assignments.
//
// Problem: Declaring a struct var via `Foo()` (or as `var a : Foo` for
// `[safe_when_uninitialized]` structs), OR a pointer-to-struct via zero-arg
// `new Foo()` / `new Foo`, then patching it with TWO OR MORE contiguous field
// assignments is the same "I-forgot-the-idiomatic-init" shape as STYLE012 —
// and the named-argument constructor reads better and avoids the redundant
// zero-init.
//
// Bad patterns:
//   var a = Foo()              var a = new Foo()
//   a.x = 1                    a.x = 1
//   a.y = 2                    a.y = 2
//
// Good patterns:
//   var a = Foo(x = 1, y = 2)
//   var a = new Foo(x = 1, y = 2)
//
// A single trailing field assignment stays silent — it is often the more
// readable form (e.g. when the struct is mostly defaulted and only one
// field is patched), and we want this rule to flag the bug case, not
// every possible rewrite. Null-pointer init (`var a : Foo?`) is NOT flagged
// here — that's a guaranteed NPE on the first `a.x = 1`, a different bug.

expect 31209:8

require daslib/style_lint

struct Foo {
    x : int = 0
    y : int = 0
    s : string = ""
}

[safe_when_uninitialized]
struct Bar {
    x : int = 0
    y : int = 0
}

struct Movable {
    tag : string = ""
    payload : array<int>
}

// bad: var a = Foo() then two field assigns (1 warning)
def bad_default_ctor() {
    var a = Foo()
    a.x = 1
    a.y = 2
}

// bad: var a : Bar (uninitialized, struct is safe_when_uninitialized) (1 warning)
def bad_uninit() {
    var a : Bar
    a.x = 3
    a.y = 4
}

// bad: three contiguous assigns still produce a single warning (1 warning)
def bad_three_fields() {
    var a = Foo()
    a.x = 1
    a.y = 2
    a.s = "hi"
}

// bad: clone-assign on string field (1 warning, via ExprClone branch)
def bad_clone_assign() {
    var a = Foo()
    a.x = 1
    a.s := "via_clone"
}

// bad: move-assign on a non-copyable field (1 warning, via ExprMove branch)
def bad_move_assign() {
    var a = Movable()
    a.tag = "lbl"
    a.payload <- [1, 2, 3]
}

// good: single assign — silent, often the more readable form
def good_single_field() {
    var a = Foo()
    a.x = 42
}

// good: named-argument constructor — the idiomatic form
def good_named_ctor() {
    let a = Foo(x = 1, y = 2, s = "ok")
    let _ = a.x
}

// good: partial constructor + one trailing assign — init is non-empty,
// so STYLE013 stays silent (and a single trailing patch is acceptable)
def good_partial_ctor() {
    var a = Foo(x = 1)
    a.y = 2
}

// good: inscope variable — matches STYLE011/012 exclusion
def good_inscope() {
    var inscope a = Foo()
    a.x = 1
    a.y = 2
}

// good: assignments split by an intervening statement — not contiguous
def good_interleaved() : int {
    var a = Foo()
    let stride = 10
    a.x = stride
    return a.x
}

// good: modifying an existing struct value (init is not an empty MakeStruct)
def make_foo : Foo {
    return Foo(x = 7)
}
def good_modify_existing() {
    var a = make_foo()
    a.x = 1
    a.y = 2
}

// ── new-allocated pointer cases ──────────────────────────────────────────────

// bad: var a = new Foo() then two field assigns (1 warning)
def bad_default_new() {
    var a = new Foo()
    a.x = 1
    a.y = 2
}

// bad: var a = new Foo (no parens) then two field assigns (1 warning)
def bad_default_new_no_parens() {
    var a = new Foo
    a.x = 1
    a.y = 2
}

// bad: mix of := clone and = copy through pointer (1 warning, via ExprClone branch)
def bad_new_clone_then_copy() {
    var a = new Foo()
    a.s := "hi"
    a.x = 1
}

// good: var a = new Foo(x=1) — non-empty new, silent
def good_partial_new_ctor() {
    var a = new Foo(x = 1)
    a.y = 2
}

// good: var a : Foo? — null init, silent (different bug; not STYLE013).
// Guarded by `if (a != null)` so we don't actually trip the NPE at test-run time
// (the linter only inspects AST shape; the guard never affects detection).
def good_null_ptr_init() {
    var a : Foo?
    if (a != null) {
        a.x = 1
        a.y = 2
    }
}

// good: var a = make_foo_ptr() — non-`new` pointer init, silent
def make_foo_ptr : Foo ? {
    return new Foo(x = 7)
}
def good_modify_existing_ptr() {
    var a = make_foo_ptr()
    a.x = 1
    a.y = 2
}
