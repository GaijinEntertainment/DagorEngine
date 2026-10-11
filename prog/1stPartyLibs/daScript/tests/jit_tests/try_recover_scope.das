options gen2
require dastest/testing_boost

def private apply_it(blk : block<(x : int) : int>) : int {
    return invoke(blk, 1)
}

def scope_in_arm : int {
    return apply_it() $(x : int) : int {
        var res = 0
        try {
            var inscope arr <- [1, 2, 3]
            res = length(arr) + x
        } recover {
            res = -1
        }
        return res
    }
}

def scope_in_arm_recovering : int {
    return apply_it() $(x : int) : int {
        var res = 0
        try {
            var inscope arr <- [1, 2, 3]
            res = length(arr) / (x - 1)
        } recover {
            res = -1
        }
        return res
    }
}

[test]
def test_try_recover_scope(t : T?) {
    t |> run("inscope in a try arm inside a block") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@scope_in_arm))
        t |> equal(4, scope_in_arm())
    }
    t |> run("the recover arm runs with the arm's scope unwound") @(t : T?) {
        t |> equal(-1, scope_in_arm_recovering())
    }
}
