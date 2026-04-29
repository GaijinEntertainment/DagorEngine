options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/safe_addr

// shared global scalar
let shared MAGIC = 42

// shared global array
let shared NAMES <- ["alpha", "beta", "gamma"]

[test]
def test_shared_addr_scalar(t : T?) {
    t |> run("address of shared int") @(tt : T?) {
        let p = shared_addr(MAGIC)
        tt |> equal(*p, 42)
    }
}

[test]
def test_shared_addr_array(t : T?) {
    t |> run("address of shared array") @(tt : T?) {
        let p = shared_addr(NAMES)
        tt |> equal((*p)[0], "alpha")
        tt |> equal((*p)[1], "beta")
        tt |> equal((*p)[2], "gamma")
    }
    t |> run("iterate shared values") @(tt : T?) {
        var count = 0
        for (name in NAMES) {
            let pn = shared_addr(name)
            tt |> equal(empty(*pn), false)
            count++
        }
        tt |> equal(count, 3)
    }
}
