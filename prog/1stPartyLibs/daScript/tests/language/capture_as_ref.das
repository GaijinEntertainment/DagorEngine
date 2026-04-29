options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

class CaptureRefFoo {
    i : int
    k : array<int>
    def CaptureRefFoo {
        i = 5
    }
    def kaboom : lambda<int> {
        var counter <- @() {
            return i++
        }
        return <- counter
    }
}

[test]
def test_capture_as_ref(t : T?) {
    t |> run("class lambda captures self as ref") @(t : T?) {
        var x = new CaptureRefFoo()
        let boom <- x->kaboom()
        t |> equal(x.i, 5)
    }
}
