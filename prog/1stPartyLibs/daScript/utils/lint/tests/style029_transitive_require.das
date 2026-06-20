options gen2
// STYLE029: non-public 'require X' used only for modules X re-exports
// (transitive-only) — require those directly and drop X.
//
// Problem: requiring a module purely as a conduit to symbols it re-exports
// publicly hides the real dependency. The proxy can change its re-exports out
// from under you, and a reader can't tell what you actually depend on.
//
// Bad pattern:
//   require proxy        // proxy does `require leaf public`
//   ... leaf_value() ... // only leaf's symbols are used
//
// Good pattern:
//   require leaf
//   ... leaf_value() ...
//
// Guards that suppress the warning (no false positives):
//   * the required module is used directly (>=1 of its own symbols referenced)
//   * the module registers a compile-time macro (pessimistic — its uses leave
//     no traceable AST node)
//   * the module has an [init] function (runs side effects on load)
//   * nothing it re-exports is used either (genuinely unused — deferred, a
//     separate concern from transitive-only)

expect 31209:1

require daslib/style_lint
require _style029_proxy        // transitive-only — only the leaf's symbol is used (FIRES)
require _style029_direct       // used directly — must NOT fire

[export]
def use_them() : int {
    // style029_leaf_value is re-exported by the proxy; style029_direct_value is
    // the direct module's own symbol.
    return style029_leaf_value() + style029_direct_value()
}
