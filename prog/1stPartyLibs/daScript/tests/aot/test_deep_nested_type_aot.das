options gen2
require dastest/testing_boost public

// Regression for #3065 / PR #3145 (AOT emitter stack bump). describeCppTypeEx recurses
// once per nesting level when emitting a type, so a deeply-nested function type overflows
// the emitter's default ~16KB macro-context stack (overflow threshold ~13 levels). That
// used to silently drop the field and emit uncompilable C++ (a Func deep; with an
// unbalanced DAS_COMMENT, plus a dangling offsetof). With options stack bumped in
// daslib/aot_cpp.das the ~40-level field below emits a complete, balanced field; the
// tests/aot harness compiles the emitted C++, so removing the bump would fail this AOT
// build. 40 is ~3x the overflow threshold yet well within the 1MB bump -- the daslang
// context stack is a platform-independent managed buffer, so depth behaves identically
// across CI targets.

struct Holder {
    deep : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : function<(a : int) : int>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

[test]
def test_deep_nested_type_aot(t : T?) {
    t |> run("deeply-nested function type AOT-emits a complete field") @(t : T?) {
        let h = Holder()
        t |> equal(h.deep == null, true)
    }
}
