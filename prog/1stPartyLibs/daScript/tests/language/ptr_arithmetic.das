options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def pa_test_signed(t : T?; dummy : auto(TT)) {
    unsafe {
        var zero = TT(0); zero--; zero++
        var one = TT(1); one--; one++
        var two = TT(2); two--; two++
        var three = TT(3); three--; three++
        var neg_one = TT(-1); neg_one--; neg_one++
        var a <- fixed_array<int>(0, 1, 2, 3)
        var pA = addr(a[1])
        t |> equal(*pA, 1)
        pA++
        t |> equal(*pA, 2)
        pA--
        t |> equal(*pA, 1)
        t |> equal(pA[zero], 1)
        t |> equal(pA[one], 2)
        t |> equal(pA[neg_one], 0)
        pA += two
        t |> equal(*pA, 3)
        pA -= three
        t |> equal(*pA, 0)
        t |> equal(*(pA + two), 2)
        pA += 2
        t |> equal(*(pA - one), 1)
    }
}

def pa_test_unsigned(t : T?; dummy : auto(TT)) {
    unsafe {
        var zero = TT(0); zero--; zero++
        var one = TT(1); one--; one++
        var two = TT(2); two--; two++
        var three = TT(3); three--; three++
        var a <- fixed_array<int>(0, 1, 2, 3)
        var pA = addr(a[1])
        t |> equal(*pA, 1)
        pA++
        t |> equal(*pA, 2)
        pA--
        t |> equal(*pA, 1)
        t |> equal(pA[zero], 1)
        t |> equal(pA[one], 2)
        pA += two
        t |> equal(*pA, 3)
        pA -= three
        t |> equal(*pA, 0)
        t |> equal(*(pA + two), 2)
        pA += 2
        t |> equal(*(pA - one), 1)
    }
}

[test]
def test_ptr_arithmetic(t : T?) {
    t |> run("signed int pointer arithmetic") @(t : T?) {
        pa_test_signed(t, type<int>)
    }
    t |> run("signed int64 pointer arithmetic") @(t : T?) {
        pa_test_signed(t, type<int64>)
    }
    t |> run("unsigned uint pointer arithmetic") @(t : T?) {
        pa_test_unsigned(t, type<uint>)
    }
    t |> run("unsigned uint64 pointer arithmetic") @(t : T?) {
        pa_test_unsigned(t, type<uint64>)
    }
}
