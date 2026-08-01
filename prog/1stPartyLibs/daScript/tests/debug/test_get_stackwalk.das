// Test: get_stackwalk - stack walk captured as a string instead of printed
options gen2

require dastest/testing_boost public
require strings
require daslib/rtti
require debugapi

def private capture_own_stack() : string => get_stackwalk()

[test]
def test_get_stackwalk(t : T?) {
    t |> run("builtin returns the stack as a string") @(t : T?) {
        let s = capture_own_stack()
        t |> success(!empty(s))
        t |> success(find(s, "CALL STACK") != -1)
    }
    t |> run("all four bools accepted") @(t : T?) {
        let s = get_stackwalk(false, false, false, true)
        t |> success(!empty(s))
        t |> success(find(s, "CALL STACK") != -1)
    }
    t |> run("debugapi variant walks a given context") @(t : T?) {
        let s = get_stackwalk(this_context(), get_line_info())
        t |> success(!empty(s))
        t |> success(find(s, "CALL STACK") != -1)
    }
}
