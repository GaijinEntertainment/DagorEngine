// Macro Tutorial 8: Variant Macros — Interface is / as / ?as
//
// This tutorial demonstrates [variant_macro] — macros that intercept
// the `is`, `as`, and `?as` operators during type inference.
//
// Previous tutorials transformed functions, structs, and blocks.
// Variant macros are different: they extend AstVariantMacro and
// override three visitor methods:
//
//   visitExprIsVariant()     — intercepts `expr is Name`
//   visitExprAsVariant()     — intercepts `expr as Name`
//   visitExprSafeAsVariant() — intercepts `expr ?as Name`
//
// The compiler calls each registered variant macro in order.  The
// first one to return a non-default ExpressionPtr wins — the operator
// is replaced with the returned expression.  If all macros return
// default, the compiler falls through to generic operator functions
// and then to built-in variant logic.
//
// Resolution order (three-tier):
//   1. Variant macros  (visitExpr*Variant returns non-default)
//   2. Generic operators  (def operator is/as Name)
//   3. Built-in variant handling  (native variant types)
//
// Unlike tutorials 01–07, this tutorial does NOT have a separate
// _mod.das file.  The variant macro lives in the standard library
// module `daslib/interfaces` (class InterfaceAsIs).  We simply
// require that module and use its `is`, `as`, and `?as` support
// for interface types.
//
// Covers:
//   - [variant_macro(name=...)] to register a variant macro
//   - AstVariantMacro with visitExprIsVariant / visitExprAsVariant /
//     visitExprSafeAsVariant
//   - Type guard pattern: checking expr.value._type in the visitor
//   - find_unique_structure to look up types at compile time
//   - qmacro for generating replacement expressions
//   - clone_expression for safe double-use of an expression
//   - [interface] and [implements] from daslib/interfaces
//
// Run: daslang.exe tutorials/macros/08_variant_macro.das

options gen2

require daslib/interfaces

// ── Interfaces ──

[interface]
class IDrawable {
    def abstract draw(x, y : int) : void
}

[interface]
class IResizable {
    def abstract resize(w, h : int) : void
}

// ── A struct that implements both interfaces ──

[implements(IDrawable), implements(IResizable)]
class Widget {
    def Widget() {
        pass
    }
    def IDrawable`draw(x, y : int) {
        print("Widget.draw at ({x},{y})\n")
    }
    def IResizable`resize(w, h : int) {
        print("Widget.resize to {w}x{h}\n")
    }
}

// ── A struct that implements only IDrawable ──

[implements(IDrawable)]
class Label {
    text : string
    def Label(t : string) {
        text = t
    }
    def IDrawable`draw(x, y : int) {
        print("Label \"{text}\" at ({x},{y})\n")
    }
}

// ── Demonstrate is / as / ?as ──

[export]
def main() {
    var w = new Widget()
    var l = new Label("hello")

    // ─── is ─── compile-time interface check
    //
    // `w is IDrawable` is replaced at compile time with `true`
    // because Widget has [implements(IDrawable)].
    // `l is IResizable` becomes `false` — Label does not implement it.
    print("w is IDrawable  = {w is IDrawable}\n")   // true
    print("w is IResizable = {w is IResizable}\n")   // true
    print("l is IDrawable  = {l is IDrawable}\n")    // true
    print("l is IResizable = {l is IResizable}\n")   // false

    // ─── as ─── get interface proxy
    //
    // `w as IDrawable` calls the generated getter function
    // Widget`get`IDrawable(*w), which returns an IDrawable? proxy.
    var drawable = w as IDrawable
    drawable->draw(10, 20)

    var resizable = w as IResizable
    resizable->resize(800, 600)

    // ─── ?as ─── null-safe interface access
    //
    // When the pointer is non-null, ?as calls the getter — same as `as`.
    // When the pointer is null, ?as returns null without crashing.
    var maybe_draw = l ?as IDrawable
    if (maybe_draw != null) {
        maybe_draw->draw(5, 5)
    }

    // null pointer — ?as returns null safely
    var nothing : Label?
    var safe = nothing ?as IDrawable
    print("null ?as IDrawable = {safe}\n")  // null

    unsafe {
        delete w
        delete l
    }
}

// expected output:
// w is IDrawable  = true
// w is IResizable = true
// l is IDrawable  = true
// l is IResizable = false
// Widget.draw at (10,20)
// Widget.resize to 800x600
// Label "hello" at (5,5)
// null ?as IDrawable = null
