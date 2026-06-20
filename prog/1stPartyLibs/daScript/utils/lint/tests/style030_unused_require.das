options gen2
// STYLE030: non-public 'require X' that is entirely unused — no symbol from X
// (or anything it re-exports) is referenced. Drop it.
//
// This is the fully-unused sibling of STYLE029 (transitive-only). The same
// pessimistic guards suppress false positives:
//   * direct use of any of X's own symbols
//   * X provides any compile-time macro (call/reader/variant/for_loop/typeinfo/
//     type/pass/capture/simulate) — fires by syntax / installs a pass, leaving
//     no traceable symbol reference
//   * X has an [init] function (side effects on load)
//   * builtin facade re-exports (no source file) don't count as "used"
//
// Without a recompile-verify (which a lint pass can't do), the macro guard is
// what makes this safe: e.g. `require daslib/style_lint` below installs a
// [lint_macro] and references nothing, yet must NOT be flagged.

expect 31209:1

require daslib/style_lint       // [lint_macro] module — pass-macro guard must NOT flag it
require _style030_unused        // genuinely unused source module (FIRES)
require _style029_direct        // used directly — must NOT fire

[export]
def use_them() : int {
    return style029_direct_value()
}
