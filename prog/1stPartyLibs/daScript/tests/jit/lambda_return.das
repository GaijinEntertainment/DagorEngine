options gen2
options indenting = 4

require dastest/testing_boost

typedef MyFn = lambda<(x : int) : int>

def make_fn() : MyFn {
    var f <- @(x : int) : int { return x * 2; }
    return <- f
}

def make_fn_capture(n : int) : MyFn {
    var cn = n
    return @capture(<- cn) (x : int) : int {
        return x * cn
    }
}

def wrap_fn(var fn : MyFn) : MyFn {
    return <- fn
}

def double_wrap(var fn : MyFn) : MyFn {
    return <- wrap_fn(fn)
}

[test]
def test_lambda_return(t : T?) {
    // basic: return <- lambda from function
    var f <- make_fn()
    t |> equal(f(5), 10)
}

[test]
def test_lambda_return_with_capture(t : T?) {
    // return <- lambda with captured variable
    var f <- make_fn_capture(3)
    t |> equal(f(5), 15)
}

[test]
def test_lambda_return_chain(t : T?) {
    // lambda returned through two function calls
    var f <- wrap_fn(make_fn())
    t |> equal(f(5), 10)
}

[test]
def test_lambda_return_double_chain(t : T?) {
    // lambda returned through three function calls
    var f <- double_wrap(make_fn())
    t |> equal(f(5), 10)
}

[test]
def test_lambda_return_capture_chain(t : T?) {
    var f <- wrap_fn(make_fn_capture(7))
    t |> equal(f(4), 28)
}
