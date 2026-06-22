options gen2
require dastest/testing_boost public

// Coverage for lambda being a copyable, pointer-shaped value.
// canCopy(lambda) is now true: '=' aliases the capture frame.
// isSafeToDelete(lambda) is now false: 'delete lam' requires unsafe.

typedef Counter = lambda<() : int>

def invoke_twice(g : Counter) : int {
    return invoke(g) + invoke(g)
}

[test]
def test_lambda_copy_aliases_capture(t : T?) {
    t |> run("copy assignment shares capture frame") @(t : T?) {
        var counter = 0 // nolint:LINT003 -- captured by lambda, mutated inside body
        var inc <- @() : int {
            counter += 1   // mutates the lambda's own captured counter
            return counter
        }
        let inc2 = inc     // copy — aliases the same capture frame
        t |> equal(invoke(inc), 1)
        t |> equal(invoke(inc2), 2)    // sees inc's mutation through the alias
        t |> equal(invoke(inc), 3)     // and vice versa
        unsafe { delete inc; }
    }
}

[test]
def test_lambda_pass_by_value(t : T?) {
    t |> run("lambda passes by value (one capture frame, two refs)") @(t : T?) {
        var lam <- @() : int {
            var x = 0
            x += 1
            return x
        }
        // pass-by-value: the parameter is a separate variable holding the same pointer
        let sum = invoke_twice(lam)
        t |> equal(sum, 2)             // each invoke returns 1 (independent x), sum = 2
        unsafe { delete lam; }
    }
}

struct Holder {
    action : Counter
}

[test]
def test_lambda_struct_field_copy(t : T?) {
    t |> run("struct copy aliases lambda field") @(t : T?) {
        var s1 = Holder(action <- @() : int {
            var n = 0
            n += 1
            return n
        })
        let s2 = s1                    // copies the struct; lambda field is aliased
        t |> equal(invoke(s1.action), 1)
        t |> equal(invoke(s2.action), 1)   // independent n per invoke
        unsafe { delete s1.action; }
    }
}

[test]
def test_lambda_implicit_capture(t : T?) {
    t |> run("inner lambda captured implicitly inside another lambda") @(t : T?) {
        // Pre-flip this required an explicit `@capture(<- inner)` annotation
        // (lambda was canCopy=false, so default `capture_any` forced move and
        // implicit move-capture is an `unsafe` operation). Post-flip the
        // lambda value is a pointer-alias, copy-capture is fine implicitly.
        var inner <- @() : int {
            var n = 0
            n += 1
            return n
        }
        var outer <- @() : int {
            return invoke(inner) + invoke(inner)
        }
        t |> equal(invoke(outer), 2)
        unsafe {
            delete inner
            delete outer
        }
    }
}
