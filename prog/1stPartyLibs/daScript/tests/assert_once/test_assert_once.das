options gen2

require daslib/assert_once
require dastest/testing_boost public

[test]
def test_assert_once(t : T?) {
    var ii = 0
    var count = 0
    label 0:
    try {
        assert_once(ii == 0)
    } recover {
        count++
    }
    ii++
    if (ii != 5) {
        goto 0
    }
    t |> equal(count, 1)
}
