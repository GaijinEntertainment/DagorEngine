options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public

// Helper: accepts a block, calls it, returns the result
def call_block(blk : block<() : int>) : int {
    return invoke(blk)
}

// Helper: accepts a block with an argument
def call_block_with_arg(value : int; blk : block<(v : int) : int>) : int {
    return invoke(blk, value)
}

// Class with a method that accepts a block
class Receiver {
    value : int = 100
    def call_method(blk : block<() : int>) : int {
        return invoke(blk)
    }
    def call_method_with_arg(extra : int; blk : block<(v : int) : int>) : int {
        return invoke(blk, extra)
    }
}

// Struct with a lambda field for -> pipe testing
struct Callable {
    fn : lambda<(blk : block<() : int>) : int>
}

def make_callable() : Callable {
    return Callable(fn = @(blk : block<() : int>) : int {
        return invoke(blk);
    })
}

// ============================================================
// 1. Named function call (expr_call) + assumed pipe
// ============================================================

[test]
def test_named_call_bare_block(t : T?) {
    t |> run("named_call() bare block") @(t : T?) {
        let res = call_block() {
            return 42;
        }
        t |> equal(res, 42)
    }
}

[test]
def test_named_call_block(t : T?) {
    t |> run("named_call() block") @(t : T?) {
        let res = call_block() $ {
            return 42;
        }
        t |> equal(res, 42)
    }
    t |> run("named_call() block with args") @(t : T?) {
        let res = call_block_with_arg(10) $(v : int) : int {
            return v + 5;
        }
        t |> equal(res, 15)
    }
}

[test]
def test_named_call_lambda_block(t : T?) {
    t |> run("named_call() @ block") @(t : T?) {
        let res = call_block() $ {
            return 42;
        }
        t |> equal(res, 42)
    }
    t |> run("named_call() lambda with args") @(t : T?) {
        let res = call_block_with_arg(10) $(v : int) : int {
            return v + 5;
        }
        t |> equal(res, 15)
    }
}

[test]
def test_named_call_explicit_pipe(t : T?) {
    t |> run("named_call() <| $ block") @(t : T?) {
        let res = call_block() <| $ {
            return 42;
        }
        t |> equal(res, 42)
    }
    t |> run("named_call() <| block with args") @(t : T?) {
        let res = call_block_with_arg(10) <| $(v : int) : int {
            return v + 5;
        }
        t |> equal(res, 15)
    }
}

// ============================================================
// 2. Arrow method call (expr_method_call) + assumed pipe
// ============================================================

[test]
def test_arrow_call_bare_block(t : T?) {
    t |> run("obj->method() bare block") @(t : T?) {
        var c = make_callable()
        let res = c->fn() {
            return 77;
        }
        t |> equal(res, 77)
    }
}

[test]
def test_arrow_call_block(t : T?) {
    t |> run("obj->method() block") @(t : T?) {
        var c = make_callable()
        let res = c->fn() $ {
            return 77;
        }
        t |> equal(res, 77)
    }
}

[test]
def test_arrow_call_lambda_block(t : T?) {
    t |> run("obj->method() @ block") @(t : T?) {
        var c = make_callable()
        let res = c->fn() $ {
            return 77;
        }
        t |> equal(res, 77)
    }
}

[test]
def test_arrow_call_explicit_pipe(t : T?) {
    t |> run("obj->method() <| $ block") @(t : T?) {
        var c = make_callable()
        let res = c->fn() <| $ {
            return 77;
        }
        t |> equal(res, 77)
    }
}

// ============================================================
// 3. Dot method call (expr_field) + assumed pipe
// ============================================================

[test]
def test_dot_call_bare_block(t : T?) {
    t |> run("obj.method() bare block") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method() {
            return 33;
        }
        t |> equal(res, 33)
    }
}

[test]
def test_dot_call_block(t : T?) {
    t |> run("obj.method() block") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method() $ {
            return 33;
        }
        t |> equal(res, 33)
    }
    t |> run("obj.method() block with args") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method_with_arg(10) $(v : int) : int {
            return v + 23;
        }
        t |> equal(res, 33)
    }
}

[test]
def test_dot_call_lambda_block(t : T?) {
    t |> run("obj.method() @ block") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method() $ {
            return 33;
        }
        t |> equal(res, 33)
    }
    t |> run("obj.method() lambda with args") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method_with_arg(10) $(v : int) : int {
            return v + 23;
        }
        t |> equal(res, 33)
    }
}

[test]
def test_dot_call_explicit_pipe(t : T?) {
    t |> run("obj.method() <| $ block") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method() <| $ {
            return 33;
        }
        t |> equal(res, 33)
    }
    t |> run("obj.method() <| block with args") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method_with_arg(10) <| $(v : int) : int {
            return v + 23;
        }
        t |> equal(res, 33)
    }
}

// ============================================================
// 4. Pipe operator (|>) + assumed pipe
// ============================================================

[test]
def test_rpipe_block(t : T?) {
    t |> run("arg |> func() block with args") @(t : T?) {
        let res = 10 |> call_block_with_arg() $(v : int) : int {
            return v * 2;
        }
        t |> equal(res, 20)
    }
}

// ============================================================
// 5. Mixed: extra args before the block
// ============================================================

[test]
def test_named_call_with_extra_args(t : T?) {
    t |> run("named_call(arg) block with args") @(t : T?) {
        let res = call_block_with_arg(7) $(v : int) : int {
            return v * 3;
        }
        t |> equal(res, 21)
    }
}

[test]
def test_dot_call_with_extra_args(t : T?) {
    t |> run("obj.method(arg) block with args") @(t : T?) {
        var r = new Receiver()
        let res = r.call_method_with_arg(5) $(v : int) : int {
            return v + 50;
        }
        t |> equal(res, 55)
    }
}
