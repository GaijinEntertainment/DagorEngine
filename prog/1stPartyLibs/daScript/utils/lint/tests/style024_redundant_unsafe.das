options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
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
//   on variant, ExprCallFunc whose `func.flags.unsafeOperation` (or
//   unsafeOutsideOfFor outside a for-loop source) is set, an ExprMove of
//   a class or of a value that needs inscope, and a non-inscope local
//   whose type needs inscope. If nothing matches, the wrap is redundant.
//
// Note: the parser-flag form (`unsafe(expr)`) only survives compile-time
// folding when `no_optimizations + no_infer_time_folding` is set; the
// lint runner enables both. Constant-foldable inner expressions therefore
// only trip STYLE024 under `utils/lint/main.das`. The block form fires
// under regular compile too.

expect 31209:6

require daslib/style_lint
require daslib/rtti

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
    let p = unsafe(reinterpret<int?>(unsafe(addr(f))))  // nolint:STYLE034 — wrap justified (reinterpret cast); raw shape kept to exercise the recognizer
    print("{*p}\n")
}

def good_block_with_two_unsafe_ops() : void {
    var f = Foo(x = 5)
    var g = Foo(x = 7)
    unsafe {
        let pa = reinterpret<int?>(addr(f))           // nolint:STYLE034 — reinterpret + addr need unsafe
        let pb = reinterpret<int?>(addr(g))           // nolint:STYLE034 — second one — STYLE025 silent (≥2)
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

// --- Regression: `?[]` on a raw pointer requires unsafe ---
// `p?[i]` with p : T? is unsafe_pointer_safe_index (ast_infer_type.cpp:3442)
// for ANY non-vector, non-fixed-array pointee — including plain scalars.
// STYLE024 must NOT fire (found via #3391's AOT regression test, whose
// scalar-pointee safe-index wraps were falsely flagged).

def good_safe_index_of_scalar_pointer(p : float?; i : int64) : float {
    return unsafe(p?[i]) ?? -1.0
}

struct private Node {
    a : int
}

def good_unsafe_block_delete_pointer() : void {
    var p = new Node(a = 1)
    unsafe {
        delete p
    }
}

def bad_unsafe_block_delete_container() : void {
    var arr : array<int>
    arr |> push(1)
    unsafe {                                          // STYLE024 - a plain container is safe to delete
        delete arr
    }
}

// --- Regression: a reference bound to a non-local expression requires unsafe ---
// `var r & = g_rack.items[i]` binds a local reference into storage a global pointer owns
// (ast_infer_type.cpp's isLocalOrGlobal check), so a block whose only unsafe need is that
// binding is NOT redundant: the compiler refuses the binding outside `unsafe`. STYLE024 must
// NOT fire (the block frame carries the let-ref mark without an unsafe-op count).

struct private Rack {
    items : array<int>
}

var private g_rack : Rack?

def good_block_binds_non_local_ref(i : int) : void {
    unsafe {
        var r & = g_rack.items[i]
        r += 1
    }
}

// --- Regression: moving a value that holds a smart pointer requires unsafe ---
// infer refuses `a <- b` outside `unsafe` when b's type needs inscope (strict_smart_pointers is
// on by default), and `unsafe(a <- b)` does not parse, so the block form is the only spelling:
// neither STYLE024 nor STYLE025 may fire on it.

struct private SmartHolder {
    ctx : smart_ptr<Context>
}

var private g_holder : SmartHolder
var private g_holder_src : SmartHolder

def good_block_moves_smart_pointer_holder() : void {
    unsafe {
        g_holder <- g_holder_src
    }
}

// --- Regression: moving classes, and declaring a smart-pointer holder local, require unsafe ---
// infer refuses `a <- b` on class references outside `unsafe`, and a local whose type needs
// inscope must be declared `inscope` or under `unsafe`: neither STYLE024 nor STYLE025 may fire.

class private Widget {
    id : int
}

def good_block_moves_class_refs(var a : Widget&; var b : Widget&) : void {
    unsafe {
        a <- b
    }
}

def good_block_declares_smart_holder_local() : void {
    unsafe {
        var local : SmartHolder
        g_holder <- local
    }
}
