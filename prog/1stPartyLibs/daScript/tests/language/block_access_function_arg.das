options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def addValue(a, blk) {
    return invoke(blk, a)
}

[test]
def test_block_access_function_arg(t : T?) {
    t |> run("nested block accesses outer lambda var") @(t : T?) {
        var CNT = 0
        let counter <- @(extra : int) : int {
            let e1 = addValue(1) <| $(extraBlock : int) : int {
                return extraBlock + CNT
            }
            return CNT++ + e1
        }
        t |> equal(invoke(counter, 0), 1)
    }
}
