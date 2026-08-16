options gen2

require dastest/testing_boost
require daslib/lint

// LINT002 false positive: invoke devirtualization splices the block literal in place and eats
// the holder's only read. Lint runs after the patch slot but before optimize reaps the dead
// `let`, so it saw a variable with no access flag and reported error[50503]. The devirtualizer
// now marks such holders marked_used. A function-pointer local never devirtualizes, so it was
// always clean - that asymmetry is what made this look block-typed-specific.

def apply_via_invoke(x : int) : int {
    let add_one = $(v : int) : int { return v + 1 }
    return invoke(add_one, invoke(add_one, x))
}

def apply_block(blk : block<(v : int) : int>; x : int) : int {
    return invoke(blk, x)
}

def apply_via_call(x : int) : int {
    let add_two = $(v : int) : int { return v + 2 }
    return apply_block(add_two, x)
}

[test]
def test_block_var_read_by_invoke(t : T?) {
    t |> equal(apply_via_invoke(1), 3)
}

[test]
def test_block_var_passed_to_function(t : T?) {
    t |> equal(apply_via_call(1), 3)
}
