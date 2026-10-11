options gen2

require dastest/testing_boost public

// Regression: a generator body calling a function that takes a BLOCK argument stopped
// compiling. The inliner binds a block-literal argument to a manufactured local holder
// whenever the callee reads the parameter more than once or under a loop (ast_inline.cpp,
// makeArgTemp) - and inside a generator every top-level local is lifted into the generator's
// capture structure, which a block can never live in. The result was a pair of errors against
// generated code the author cannot act on:
//   error[30239]: _lambda_thismodule_N_1() can't be allocated locally ... contains Handled type
//   error[30914]: type block<(v:int const):int const>& can't be cloned from block<...> const
// The site now declines inside a generator (the call stays a call), and hand-written block
// locals in a generator report invalid_local_type (30202) instead of the pair above.

def private transform_sum(n : int; blk : block<(v : int) : int>) : int {
    // reads the block under a loop, which is what forces the holder path
    var total = 0
    for (v in range(n)) {
        total += invoke(blk, v)
    }
    return total
}

def private pick(blk : block<(v : int) : int>) : int {
    // reads the block twice, the other holder trigger
    return invoke(blk, 1) + invoke(blk, 2)
}

[test]
def test_generator_block_argument(t : T?) {
    t |> run("block literal under a callee loop") @(t : T?) {
        var g <- generator<int> {
            yield transform_sum(4, $(v : int) { return v * 2; })
            yield 7
            return false
        }
        let got <- [for (x in g); x]
        t |> equal(got, [12, 7])
    }
    t |> run("block literal read twice by the callee") @(t : T?) {
        var g <- generator<int> {
            yield pick($(v : int) { return v * 10; })
            return false
        }
        let got <- [for (x in g); x]
        t |> equal(got, [30])
    }
    t |> run("block argument inside a generator loop") @(t : T?) {
        var g <- generator<int> {
            for (i in range(3)) {
                yield transform_sum(i, $(v : int) { return v + 1; })
            }
            return false
        }
        let got <- [for (x in g); x]
        t |> equal(got, [0, 1, 3])
    }
}
