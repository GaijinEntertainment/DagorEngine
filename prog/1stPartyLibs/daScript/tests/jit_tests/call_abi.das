options gen2
require dastest/testing_boost

// bool is a 4-byte slot in memory but i1 in a signature, so a bool that reaches a call argument
// or a return through a variable (a load, not a compare) needs narrowing to the parameter type.

struct Box {
    x : int
    y : int
}

def take_bool(b : bool) : int {
    return b ? 1 : 0
}

[sideeffects]
def take_two_bools(a, b : bool) : bool {
    return a && b
}

[sideeffects]
def pass_loaded_bool(v : bool) : int {
    var stored = v  // nolint:LINT003 - const propagates, bypassing the store/load this exercises
    return take_bool(stored)
}

[sideeffects]
def return_loaded_bool(v : bool) : bool {
    var stored = v  // nolint:LINT003 - const propagates, bypassing the store/load this exercises
    return stored
}

[sideeffects]
def return_compared_bool(a, b : int) : bool {
    return a < b
}

def take_box(b : Box) : int {
    return b.x + b.y
}

// a temporary passed where the parameter is taken by reference
[sideeffects]
def pass_temp_struct(x, y : int) : int {
    return take_box(Box(x = x, y = y))
}

[test]
def test_call_abi(t : T?) {
    // without this the cases can silently run interpreted, and would pass either way
    t |> success(!jit_enabled() || is_jit_function(@@pass_loaded_bool))
    t |> success(!jit_enabled() || is_jit_function(@@return_loaded_bool))
    t |> success(!jit_enabled() || is_jit_function(@@return_compared_bool))
    t |> success(!jit_enabled() || is_jit_function(@@take_two_bools))
    t |> success(!jit_enabled() || is_jit_function(@@pass_temp_struct))
    t |> equal(1, pass_loaded_bool(true))
    t |> equal(0, pass_loaded_bool(false))
    t |> success(return_loaded_bool(true))
    t |> success(!return_loaded_bool(false))
    t |> success(return_compared_bool(1, 2))
    t |> success(!return_compared_bool(2, 1))
    t |> success(take_two_bools(true, true))
    t |> success(!take_two_bools(true, false))
    t |> equal(7, pass_temp_struct(3, 4))
}
