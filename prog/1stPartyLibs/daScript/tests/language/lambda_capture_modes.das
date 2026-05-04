options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct LcmFoo {
    dummy : int
}

var g_lcm_counter = 0

def operator delete(f : LcmFoo) {
    g_lcm_counter++
}

[test]
def test_lambda_capture_modes(t : T?) {
    t |> run("capture ref, move, clone") @(t : T?) {
        var a1 <- array<int>(1, 2)
        var a2 <- array<int>(1, 2)
        var a3 <- array<int>(1, 2)
        unsafe {
            var lam <- @ capture(& a1, <- a2, := a3) {
                push(a1, 1)
                push(a2, 1)
                push(a3, 1)
            }
            invoke(lam)
        }
        t |> equal(length(a1), 3)
        t |> equal(length(a2), 0)
        t |> equal(length(a3), 2)
    }
    t |> run("capture move and clone with delete") @(t : T?) {
        g_lcm_counter = 0
        var pA = new LcmFoo
        var pB = new LcmFoo
        var C = LcmFoo()
        var lam <- @ capture(<- pB, := pC) {
            assert(pA.dummy == 0)
            assert(pB.dummy == 0)
            assert(C.dummy == 0)
        }
        t |> equal(pB == null, true)
        invoke(lam)
        delete lam
        t |> equal(g_lcm_counter, 1)
    }
}
