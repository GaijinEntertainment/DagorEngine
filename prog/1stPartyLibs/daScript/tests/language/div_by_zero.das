options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[sideeffects, no_jit]
def div_by_zero(num : auto(numT)) : numT {
    var ex = false
    try {
        return num / numT(0)
    } recover {
        ex = true
    }
    assert(ex)
    return num
}

[sideeffects, no_jit]
def mod_by_zero(num : auto(numT)) : numT {
    var ex = false
    try {
        return num % numT(0)
    } recover {
        ex = true
    }
    assert(ex)
    return num
}

[test]
def test_div_by_zero(t : T?) {
    t |> run("int div and mod") @(t : T?) {
        t |> equal(div_by_zero(1), 1)
        t |> equal(mod_by_zero(1), 1)
    }
    t |> run("uint div and mod") @(t : T?) {
        t |> equal(div_by_zero(1u), 1u)
        t |> equal(mod_by_zero(1u), 1u)
    }
    t |> run("int64 div and mod") @(t : T?) {
        t |> equal(div_by_zero(1l), 1l)
        t |> equal(mod_by_zero(1l), 1l)
    }
    t |> run("uint64 div and mod") @(t : T?) {
        t |> equal(div_by_zero(1ul), 1ul)
        t |> equal(mod_by_zero(1ul), 1ul)
    }
}
