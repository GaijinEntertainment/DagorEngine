// with_module_is_unsafe policy: user-written with (module foo) compiles when
// wrapped in unsafe (the bare form is tests/typer_errors/failed_with_module_unsafe.das)
options gen2
options with_module_is_unsafe = true

require dastest/testing_boost public
require _with_module_helper

[test]
def test_with_module_under_policy(t : T?) {
    var r : int
    unsafe {
        with (module _with_module_helper) {
            r = secret_add(20, 22)
        }
    }
    t |> equal(r, 42)
}
