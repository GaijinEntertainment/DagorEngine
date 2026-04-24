options gen2
require dastest/testing_boost public

[test]
def test_block(t : T?) {
    var blk = $(a, b : int) {
        return a + b
    }
    t |> equal(3, invoke(blk, 1, 2))
    t |> equal(7, invoke(blk, 3, 4))
}




