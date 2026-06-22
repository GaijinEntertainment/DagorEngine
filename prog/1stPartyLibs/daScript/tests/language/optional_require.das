options gen2
require dastest/testing_boost public

// Optional require: `require ?guard target` loads *target* only when module *guard* is available.
// When *guard* is absent the require is skipped with no error — even if *target* itself does not
// exist (proven below: the second require names a nonexistent target yet this file still compiles).
require ?math math                                  // guard present (math is a core builtin) -> target loaded
require ?no_such_guard_xyz no_such_target_also_xyz  // guard absent -> skipped (no module-not-found error)

[test]
def test_optional_require_guard(t : T?) {
    // builtin_module_exists is the same availability primitive the require guard uses.
    t |> success(typeinfo builtin_module_exists(math))
    t |> success(!typeinfo builtin_module_exists(no_such_guard_xyz))
    // static_if over an absent guard drops its branch *before name resolution*, so the undefined
    // symbol below is never resolved. This is the invariant that lets a guarded require pair safely
    // with a guarded use of the (possibly-unloaded) target's symbols.
    static_if (typeinfo builtin_module_exists(no_such_guard_xyz)) {
        undefined_symbol_in_dead_branch_xyz()
    } else {
        t |> success(true)
    }
}
