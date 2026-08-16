options gen2
options indenting = 4
options rtti

require dastest/testing_boost public
require _test_collect_deps_empty_helper

//! Regression: collect_dependencies on a function with NO function-deps AND NO
//! variable-deps used to leave the dep arrays uninitialised (Array.magic == garbage), so
//! the das-side `for (... in ...)` lock-magic check tripped "array magic mismatch on lock"
//! and the macro_function callers (dasGlsl, dasSpirv) crashed at the first iteration.
//! Fixed in src/simulate/simulate_fn_hash.cpp by calling array_mark_locked unconditionally
//! with a (nullptr, 0) pair when the dep list is empty. The helper module's
//! [probe_empty_deps] annotation calls collect_dependencies inside its patch() — before the
//! fix that crashed the compile pass; after the fix the patch returns cleanly and the test
//! file itself compiles + runs.

// A function whose body references neither a function nor a global. patch() of
// [probe_empty_deps] runs at compile time and exercises the (now-fixed) zero-dep
// collect_dependencies path. The pre-fix daslang panics here before any test runs; the
// post-fix daslang compiles + arrives at the assertion below.
[probe_empty_deps]
def private trivial_no_deps {
}

[test]
def test_collect_deps_empty_no_crash(t : T?) {
    // If we got this far, [probe_empty_deps]'s collect_dependencies block iterated empty
    // function + variable arrays without tripping the lock-magic check. The pre-fix
    // daslang would have aborted the compile at the patch() `for (_ in vfun)` line above.
    t |> success(true, "collect_dependencies on a no-dep function did not crash at compile time")
}
