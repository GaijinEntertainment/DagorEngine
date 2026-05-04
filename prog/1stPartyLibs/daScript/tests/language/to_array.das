options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_to_array(t : T?) {
    t |> run("from fixed_array") @(t : T?) {
        let d = fixed_array<int>(1, 2, 3, 4)
        let a <- to_array(fixed_array<int>(1, 2, 3, 4))
        for (x, y in d, a) {
            t |> equal(x, y)
        }
    }
    t |> run("from range via each") @(t : T?) {
        for (x, y in range(5), each(range(5))) {
            t |> equal(x, y)
        }
        for (x, y in range(5), to_array(each(range(5)))) {
            t |> equal(x, y)
        }
    }
    t |> run("from static array") @(t : T?) {
        let d = fixed_array<int>(1, 2, 3, 4)
        unsafe {
            for (x, y in d, each(d)) {
                t |> equal(x, y)
            }
        }
        for (x, y in d, to_array(d)) {
            t |> equal(x, y)
        }
        unsafe {
            for (x, y in d, to_array(each(d))) {
                t |> equal(x, y)
            }
        }
    }
    t |> run("from dynamic array") @(t : T?) {
        let a <- to_array(fixed_array<int>(1, 2, 3, 4))
        unsafe {
            for (x, y in a, each(a)) {
                t |> equal(x, y)
            }
        }
        unsafe {
            for (x, y in a, to_array(each(a))) {
                t |> equal(x, y)
            }
        }
    }
}
