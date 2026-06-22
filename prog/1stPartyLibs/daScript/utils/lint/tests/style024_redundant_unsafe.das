options gen2
// STYLE024: redundant `unsafe` wrap; nothing inside requires unsafe
//
// Problem: writing `unsafe(expr)` or `unsafe { ... }` around code that
// has no inherently-unsafe operation adds noise and masks the *real*
// unsafe sites elsewhere in the file. The compiler still has to walk
// the wrap; the reader has to read it. Drop it.
//
// Bad pattern:
//   let c = unsafe(a + b)
//   unsafe {
//       let d = a + b
//       print("{d}\n")
//   }
//
// Good pattern:
//   let c = a + b
//   {
//       let d = a + b
//       print("{d}\n")
//   }
//
// Detection: the parser sets `genFlags.userSaidItsSafe = true` on the
// inner expression of `unsafe(expr)`, and produces an `ExprUnsafe` node
// for `unsafe { ... }`. The visitor's `expr_needs_unsafe` recognizer
// walks the body (skipping `genFlags.generated` subtrees per design) and
// looks for known inherently-unsafe AST shapes:
//   ExprCast(upcast|reinterpret), ExprDelete, ExprAddr, ExprRef2Ptr,
//   ExprAsVariant, ExprSafeAsVariant (`?as`), ExprAt on table, ExprField
//   on variant, and ExprCallFunc whose `func.flags.unsafeOperation` (or
//   unsafeOutsideOfFor outside a for-loop source) is set. If nothing
//   matches, the wrap is redundant.
//
// Note: the parser-flag form (`unsafe(expr)`) only survives compile-time
// folding when `no_optimizations + no_infer_time_folding` is set; the
// lint runner enables both. Constant-foldable inner expressions therefore
// only trip STYLE024 under `utils/lint/main.das`. The block form fires
// under regular compile too.

expect 31209:5

require daslib/style_lint

// --- Bad patterns ---

def bad_unsafe_addition(a, b : int) : int {
    return unsafe(a + b)                              // STYLE024 — wrap is decorative
}

def bad_unsafe_block_print() : void {
    let a = 1
    let b = 2
    unsafe {                                          // STYLE024 — block has no unsafe-needing stmt
        let d = a + b
        print("{d}\n")
    }
}

def bad_unsafe_string_add(s : string) : string {
    return unsafe(s + "tail")                         // STYLE024 — string concat is safe
}

def bad_unsafe_table_read(var t : table<string; int>) : bool {
    return unsafe(key_exists(t, "k"))                 // STYLE024 — key_exists is safe
}

def bad_unsafe_block_just_assignment() : int {
    var x = 0
    unsafe {                                          // STYLE024 — assignment / print are safe
        x = 5
        print("set\n")
    }
    return x
}

// --- Good patterns (no warning) ---

struct Foo {
    x : int
}

def good_reinterpret_needs_unsafe() : void {
    var f = Foo(x = 5)
    let p = unsafe(reinterpret<int?>(unsafe(addr(f))))  // wrap justified — reinterpret cast
    print("{*p}\n")
}

def good_block_with_two_unsafe_ops() : void {
    var f = Foo(x = 5)
    var g = Foo(x = 7)
    unsafe {
        let pa = reinterpret<int?>(addr(f))           // reinterpret + addr need unsafe
        let pb = reinterpret<int?>(addr(g))           // second one — STYLE025 silent (≥2)
        print("{*pa} {*pb}\n")
    }
}

// --- Regression: class on stack requires unsafe (ast_infer_type_make.cpp) ---
// `MyClass(...)` is an ExprMakeStruct with makeType.isClass — the compiler
// rejects it outside unsafe scope, so the wrap is NOT redundant. STYLE024
// must NOT fire here (Copilot review #3260356968).

class private MyClass {
    x : int
}

def good_class_on_stack() : void {
    unsafe {
        var c1 = MyClass(x = 5)
        var c2 = MyClass(x = 7)
        print("{c1.x} {c2.x}\n")
    }
}

// --- Regression: `?as` on a non-pointer variant requires unsafe ---
// `v ?as a` on a by-value variant materializes a pointer into the payload
// (ast_infer_type.cpp: unsafe_variant_safe_as), so the wrap is justified.
// STYLE024 must NOT fire (preVisitExprSafeAsVariant marks it unsafe).

variant private MyVar {
    a : int
    b : string
}

def good_safe_as_on_value_variant() : void {
    let v = MyVar(a = 5)
    let pa = unsafe(v ?as a)
    print("{pa != null}\n")
}
