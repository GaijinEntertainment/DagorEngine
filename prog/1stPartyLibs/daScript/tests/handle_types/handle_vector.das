options gen2
require dastest/testing_boost

require daslib/math_boost
require UnitTest

def operator [] (arr : Point3Array; ind : int) {
    return 2.0 * arr.[ind]
}

def test_point3_array_index(t : T?; arr : Point3Array) {
    assert(arr |> length == 10)
    for (i in range(10)) {
        let p = arr[i]
        let p_native = arr.[i]
        t |> equal(2.0, p.x)
        t |> equal(4.0, p.y)
        t |> equal(6.0, p.z)
        t |> equal(1.0, p_native.x)
        t |> equal(2.0, p_native.y)
        t |> equal(3.0, p_native.z)
    }
}

def test_point3_array_each(t : T?; arr : Point3Array) {
    assert(arr |> length == 10)
    for (p in arr) {
        t |> equal(1.0, p.x)
        t |> equal(2.0, p.y)
        t |> equal(3.0, p.z)
    }
}

def test_point3_array_block(t : T?) {
    testPoint3Array() $(arr) {
        assert(arr |> length == 10)
        for (p in arr) {
            t |> equal(1.0, p.x)
            t |> equal(2.0, p.y)
            t |> equal(3.0, p.z)
        }
    }
}

[test]
def test_handle_type(t : T?) {
    t |> run("[]") @(t : T?) {
        testPoint3Array() $(arr) {
            test_point3_array_index(t, arr)
        }
    }
    t |> run("each") @(t : T?) {
        testPoint3Array() $(arr) {
            test_point3_array_each(t, arr)
        }
    }
    t |> run("invoke block") @(t : T?) {
        test_point3_array_block(t)
    }
}
