options gen2
require dastest/testing_boost public

// Optional require: `require ?guard target` loads *target* when *guard* is available. A
// plain-name guard is STRICT: available = the guard module is registered (a linked C++
// module) — never the target's resolvability (module source dirs exist in every checkout,
// so a fallback would defeat "loaded only when <mod> is linked" guards like sql_boost's
// `require ?sqlite`). A skipped require is silent — even if *target* does not exist. The
// guard-present-but-target-missing case still errors with 20605 (verified by hand;
// module-resolution failures fire before the `expect` line parses, so that case is not
// expressible as a cant_ test).
require ?math math                                  // nolint:STYLE030 — the guarded-load mechanics ARE the test: guard present (math is a core builtin) -> target loaded
require ?no_such_guard_xyz no_such_target_also_xyz  // guard absent, target unresolvable -> skipped
require ?no_such_guard_pkg optional_require_fixture // nolint:STYLE030 — guard absent -> SKIPPED even though the target resolves (strict plain guards); the non-load IS the test
// PATH guard (contains '/'): availability = the guard's OWN file resolves; the rail for
// pure-das packages (nothing C++ to guard on) and cross-package dependency witnesses — the
// target may live in an always-present package while depending on an optional one (e.g.
// dasllama_math_gen guarding on llvm/daslib/llvm_tune).
require ?daslib/fio optional_require_fixture_pathguard        // path guard resolves -> target loaded
require ?daslib/no_such_file_xyz optional_require_fixture_skipme  // path guard unresolvable -> SKIPPED even though the target file resolves
// %/-rooted guards: grammar-legal require_module_name forms the pre-parse require scanner must
// scan identically to ast_requireModule (it once stopped at '%', mangling guard AND target)
require ?%/tests/language/optional_require_fixture.das optional_require_fixture_pctguard  // nolint:STYLE030 — %/ guard resolves -> target loaded; referenced only under static_if
require ?%/tests/language/no_such_file_xyz.das optional_require_fixture_pctskip  // %/ guard unresolvable -> SKIPPED, target file resolves

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

[test]
def test_optional_require_path_guard(t : T?) {
    // path guard resolved (daslib/fio is always mounted): the target loaded and its symbols work
    t |> equal(optional_require_fixture_pathguard::pathguard_marker(), 43)
    t |> success(typeinfo builtin_module_exists(optional_require_fixture_pathguard))
    // path guard unresolvable: the require was SKIPPED even though the target file exists on disk —
    // the pin for path-guard semantics (a plain-name guard would have loaded it via the fallback)
    t |> success(!typeinfo builtin_module_exists(optional_require_fixture_skipme))
    static_if (typeinfo builtin_module_exists(optional_require_fixture_skipme)) {
        this_symbol_must_never_resolve_xyz()
    } else {
        t |> success(true)
    }
}

[test]
def test_optional_require_pct_path_guard(t : T?) {
    // %/-rooted guards resolve only when the harness mounts the source root (the wasm suite
    // runs from a different root, where the require self-skips) — so the positive assertions
    // ride the static_if pairing. The scanner-parity pin does NOT live here: a regressed
    // collector mangles guard AND target and fails LOUDLY with 20605 at require time on
    // native lanes, before any static_if is seen.
    static_if (typeinfo builtin_module_exists(optional_require_fixture_pctguard)) {
        t |> equal(optional_require_fixture_pctguard::pctguard_marker(), 44)
    } else {
        t |> success(true)   // %/ mount absent on this harness — the require self-skipped
    }
    // %/-rooted path guard unresolvable: skipped despite the target file existing on disk
    t |> success(!typeinfo builtin_module_exists(optional_require_fixture_pctskip))
    static_if (typeinfo builtin_module_exists(optional_require_fixture_pctskip)) {
        this_symbol_must_never_resolve_pct_xyz()
    } else {
        t |> success(true)
    }
}

[test]
def test_optional_require_strict_plain_guard(t : T?) {
    // plain guard absent -> the require was SKIPPED even though the target file resolves —
    // the pin for strict plain-name guards (the old resolvability fallback loaded it, which
    // defeated "loaded only when <mod> is linked" guards on module-less builds)
    t |> success(!typeinfo builtin_module_exists(optional_require_fixture))
    static_if (typeinfo builtin_module_exists(optional_require_fixture)) {
        this_symbol_must_never_resolve_strict_xyz()
    } else {
        t |> success(true)
    }
}
