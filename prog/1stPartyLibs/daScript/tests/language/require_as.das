options gen2
require dastest/testing_boost public

// `require X as Y` must register the alias for EVERY require form. Before the fix only the
// file-path rails (./x.das, %/root.das, project module_get) set ModuleInfo::importName, and
// ast_requireModule gated alias registration on it — the daslib / builtin / same-directory
// spellings parsed the `as` clause and silently dropped it, so using the alias later died
// with a misleading 30341 "out of 0 total functions".
require math as m                          // builtin C++ module
require daslib/strings_boost as sb         // daslib mount
require require_as_fixture as fx           // same-directory bare require
require ./require_as_fixture.das as fxp    // path form (always worked): second alias, same module

[test]
def test_require_as_builtin(t : T?) {
    t |> success(m::sqrt(9.0) == 3.0)
}

[test]
def test_require_as_daslib(t : T?) {
    t |> equal(sb::join([1, 2, 3], ","), "1,2,3")
}

[test]
def test_require_as_same_dir(t : T?) {
    t |> equal(fx::require_as_marker(), 45)
    // the alias is additive — the real module name keeps working
    t |> equal(require_as_fixture::require_as_marker(), 45)
    // the path spelling of the same module aliases alongside the bare one
    t |> equal(fxp::require_as_marker(), 45)
}

[test]
def test_require_as_with_module(t : T?) {
    // `with (module <name>)` honors require aliases; locals stay lexical inside
    var joined : string
    with (module sb) {
        joined = join([4, 5], "-")
    }
    t |> equal(joined, "4-5")
}
