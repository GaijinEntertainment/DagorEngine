options gen2

require dastest/testing_boost public
require daslib/class_boost

// Verifies that [class_method] applies correctly to generic static methods
// on a struct. Before the isAppliedToGeneric flag, the [class_method]
// transformation (inject `self`, wrap body in `with(self)`) was skipped for
// generic methods because runFunctionAnnotations was not called on them.

struct Box {
    value : int = 0

    [class_method]
    def static put(v : auto) {
        value = int(v)
    }

    [class_method]
    def static get : int {
        return value
    }

    [class_method]
    def static add(v : auto) : int {
        value += int(v)
        return value
    }
}

[test]
def test_generic_class_method(t : T?) {
    t |> run("instantiate generic put with int") @(it : T?) {
        var b = Box()
        b.put(42)
        it |> equal(b.get(), 42)
    }
    t |> run("instantiate generic put with float") @(it : T?) {
        var b = Box()
        b.put(7.5)
        it |> equal(b.get(), 7)
    }
    t |> run("multiple instantiations on the same value") @(it : T?) {
        var b = Box()
        b.put(10)
        let r1 = b.add(5)
        let r2 = b.add(2.0)
        it |> equal(r1, 15)
        it |> equal(r2, 17)
        it |> equal(b.get(), 17)
    }
}
