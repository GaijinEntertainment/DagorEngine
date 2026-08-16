options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE034: reinterpret<T?>(addr(x)) collapses to addr<T?>(x)
//
// Problem: the pre-sugar spelling needs TWO unsafe gates — one for the
// addr, one for the reinterpret — producing the double-wrapped
// `unsafe(reinterpret<T?>(unsafe(addr(x))))`. The addr<T?>(x) sugar is
// the same AST with a single unsafe() covering both halves.
//
// Bad pattern:
//   let p = unsafe(reinterpret<int?>(unsafe(addr(f))))
//
// Good pattern:
//   let p = unsafe(addr<int?>(f))
//
// Pointer targets only — a pointer→integer pun (reinterpret<intptr>(addr(x)))
// has no addr<T?> spelling and stays silent. The sugar's own desugared
// output carries castFlags.fromAddrSugar and is exempt.

expect 31209:4

require daslib/style_lint

struct Foo {
    x : int
}

// --- Bad patterns ---

def bad_double_wrap() : void {
    var f = Foo(x = 5)
    let p = unsafe(reinterpret<int?>(unsafe(addr(f))))      // STYLE034
    print("{*p}\n")
}

def bad_block_form() : void {
    var a = 1.5
    var b = 2.5
    unsafe {
        let pa = reinterpret<uint?>(addr(a))                // STYLE034
        let pb = reinterpret<uint?>(addr(b))                // STYLE034 (two unsafe stmts keep STYLE025 silent)
        print("{*pa} {*pb}\n")
    }
}

def take_ptr(p : int?) : bool => p != null

def bad_arg_position() : bool {
    var f = Foo(x = 7)
    return take_ptr(unsafe(reinterpret<int?>(unsafe(addr(f)))))    // STYLE034
}

// --- Good patterns (no warning) ---

def good_sugar_itself() : void {
    var f = Foo(x = 5)
    let p = unsafe(addr<int?>(f))       // the sugar's own desugared cast must NOT re-fire
    print("{*p}\n")
}

def good_non_pointer_target() : void {
    var f = Foo(x = 5)
    let bits = unsafe(reinterpret<uint64>(unsafe(addr(f))))    // pointer→integer pun; no addr<T?> spelling
    print("{bits != 0ul}\n")
}

def good_ptr_operand(p : float?) : void {
    let q = unsafe(reinterpret<int?>(p))                // no addr underneath
    print("{q != null}\n")
}
