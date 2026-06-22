options gen2

// Fixture for test_assume_block_lint013.das.
// `assume name = <block>` stores a template block whose body reads its arguments.
// Every use of `name` substitutes a *clone* of that block, so the alias-definition
// block is never invoked directly and its arguments keep isAccessUnused == true.
// LINT013 must NOT fire here, because the arguments are genuinely used.

def call1(blk : block<(a : int) : int>) : int {
    return invoke(blk, 1)
}

def call2(blk : block<(a : int; b : int) : int>) : int {
    return invoke(blk, 1, 2)
}

[export]
def fixture_function() : int {
    assume doubler = ($(value : int) : int { return value * 2; })
    assume adder = ($(lhs : int; rhs : int) : int { return lhs + rhs; })
    return call1(doubler) + call2(adder)
}
