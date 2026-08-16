options gen2

require daslib/ast
require dastest/testing_boost public

// Consumer test for the 64-bit size accessors: validates the any_*_long_size
// bindings end-to-end from daslang. The accessors take a pointer to the
// array/table header (same idiom as convert_to_expression in ast_boost).

[test]
def test_any_array_long_size(t : T?) {
    var arr <- [10, 20, 30, 40, 50]
    unsafe {
        let p = addr<uint8 const?>(arr)
        t |> equal(any_array_long_size(p), int64(5))
    }
}

[test]
def test_any_table_long_size(t : T?) {
    var tab <- {1 => 100, 2 => 200, 3 => 300}
    unsafe {
        let p = addr<uint8 const?>(tab)
        t |> equal(any_table_long_size(p), int64(3))
    }
}
