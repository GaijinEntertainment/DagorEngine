options gen2
require UnitTest
require dastest/testing_boost public

[test]
def test_line_info(t : T?) {
    let line = testCallLine()
    t |> success(line == 7 || line == 0) // 0 with aot
}
