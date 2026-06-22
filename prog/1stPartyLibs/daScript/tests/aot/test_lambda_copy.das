options gen2
require dastest/testing_boost public

// AOT mirror of tests/language/lambda_copy.das.
// Exercises the same lambda copy + unsafe-delete coverage through the AOT codegen path.

typedef Counter = lambda<() : int>

def invoke_twice(g : Counter) : int {
    return invoke(g) + invoke(g)
}

[test]
def test_aot_lambda_copy_aliases_capture(t : T?) {
    t |> run("aot copy assignment shares capture frame") @(t : T?) {
        var counter = 0 // nolint:LINT003 -- captured by lambda, mutated inside body
        var inc <- @() : int {
            counter += 1
            return counter
        }
        let inc2 = inc
        t |> equal(invoke(inc), 1)
        t |> equal(invoke(inc2), 2)
        t |> equal(invoke(inc), 3)
        unsafe { delete inc; }
    }
}

[test]
def test_aot_lambda_pass_by_value(t : T?) {
    t |> run("aot lambda passes by value (one capture frame, two refs)") @(t : T?) {
        var lam <- @() : int {
            var x = 0
            x += 1
            return x
        }
        let sum = invoke_twice(lam)
        t |> equal(sum, 2)
        unsafe { delete lam; }
    }
}

struct Holder {
    action : Counter
}

[test]
def test_aot_lambda_struct_field_copy(t : T?) {
    t |> run("aot struct copy aliases lambda field") @(t : T?) {
        var s1 = Holder(action <- @() : int {
            var n = 0
            n += 1
            return n
        })
        let s2 = s1
        t |> equal(invoke(s1.action), 1)
        t |> equal(invoke(s2.action), 1)
        unsafe { delete s1.action; }
    }
}

[test]
def test_aot_lambda_implicit_capture(t : T?) {
    t |> run("aot inner lambda captured implicitly inside another lambda") @(t : T?) {
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
