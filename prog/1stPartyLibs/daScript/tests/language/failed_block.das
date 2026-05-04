options gen2
expect 30108, 30113

require dastest/testing_boost public

[test]
def test_fail_block(t : T?) {
    var blk = $(a, b : int) {
        return a + b
    }
    var b : block<(a : int) : void>// 30108: local block variable needs to be initialized
    let bb = blk                // 30113: local variable bb can only be initialized with make block expression
}



