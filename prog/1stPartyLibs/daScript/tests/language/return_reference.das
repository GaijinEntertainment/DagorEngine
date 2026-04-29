options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

var g_rr_t = 2

[unsafe_operation]
def rr_ret_ref() : auto& {
    var qq : auto& = g_rr_t
    unsafe {
        return qq
    }
}

[unsafe_operation]
def rr_ret_safe_ref(var a : auto&) : auto& {
    unsafe {
        return a
    }
}

def rr_add2(bl : block<int&>) {
    invoke(bl) += 2
}

[test]
def test_return_reference(t : T?) {
    t |> run("return global ref") @(t : T?) {
        g_rr_t = 2
        unsafe {
            var x = rr_ret_ref()
            var xx : int& = rr_ret_safe_ref(x)
            t |> equal(xx, 2)
        }
    }
    t |> run("assign via returned ref") @(t : T?) {
        g_rr_t = 2
        unsafe {
            var x = rr_ret_ref()
            var z : int& = rr_ret_ref()
            t |> equal(x, 2)
            t |> equal(z, 2)
            rr_ret_ref() = 3
            t |> equal(x, 2)
            t |> equal(z, 3)
        }
    }
    t |> run("block returning ref") @(t : T?) {
        g_rr_t = 2
        unsafe {
            var x = rr_ret_ref()
            var xx : int& = rr_ret_safe_ref(x)
            xx = 4
            t |> equal(x, 4)
            rr_add2() <| $() : auto& {
                unsafe {
                    return x
                }
            }
            t |> equal(x, 6)
        }
    }
}
