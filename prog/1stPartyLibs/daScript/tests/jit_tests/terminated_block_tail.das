options gen2
options optimize = false
require dastest/testing_boost

//! every shape here leaves a statement after a terminator inside one block list - the
//! optimizer would delete the tail, so the file compiles unoptimized to keep it

var g_tail_ran = 0

def bare_block_return() : int {
    {
        return 1
    }
    g_tail_ran++
    return 2
}

def constant_if_return() : int {
    if (true) {    // nolint:STYLE010 - the constant-condition if is the shape under test
        return 3
    }
    g_tail_ran++
    return 4
}

def break_in_nested_block() : int {
    var seen = 0
    for (_i in range(10)) {
        seen++
        {
            break
        }
        g_tail_ran++
    }
    return seen
}

def continue_in_nested_block() : int {
    var seen = 0
    for (_i in range(3)) {
        {
            seen++
            continue
        }
        g_tail_ran++
    }
    return seen
}

def void_bare_block_return() {
    {
        return
    }
    g_tail_ran++
}

[test]
def test_statements_after_a_terminated_nested_block(t : T?) {
    g_tail_ran = 0
    t |> equal(bare_block_return(), 1)
    t |> equal(constant_if_return(), 3)
    t |> equal(break_in_nested_block(), 1)
    t |> equal(continue_in_nested_block(), 3)
    void_bare_block_return()
    t |> equal(g_tail_ran, 0, "a statement after a terminator never runs")
}

[test]
def test_skip_shape_returns_early(t : T?) {
    if (true) {    // nolint:STYLE010 - the gated-skip shape a test file writes
        t |> skip("the early return is the shape under test")
        return
    }
    t |> failure("the tail after the early return ran")
}
