options gen2
options indenting = 4

require dastest/testing_boost public
require _find_call_macro_helper

// Regression: find_call_macro(mod, "absent_name") used to null-deref in the
// C++ binding (Module::findCall returns nullptr on miss). It now returns null,
// making it a safe soft probe.

[test]
def test_find_call_macro_missing_returns_null(t : T?) {
    t |> success(probe_find_call_macro_missing(),
                 "find_call_macro for an unregistered name should return null, not crash")
}
