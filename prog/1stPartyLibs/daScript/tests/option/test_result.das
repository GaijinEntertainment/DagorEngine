options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/result
require strings

// ===========================
// constructors + predicates
// ===========================

[test]
def test_constructors(t : T?) {
    t |> run("ok carries the value") @(t : T?) {
        let r = ok(42, type<string>)
        t |> success(r._is_ok)
        t |> equal(r._value, 42)
    }

    t |> run("err carries the error") @(t : T?) {
        let r = err("boom", type<int>)
        t |> success(!r._is_ok)
        t |> equal(r._error, "boom")
    }

    t |> run("is_ok / is_err") @(t : T?) {
        t |> success(ok(1, type<string>) |> is_ok)
        t |> success(!(err("e", type<int>) |> is_ok))
        t |> success(err("e", type<int>) |> is_err)
        t |> success(!(ok(1, type<string>) |> is_err))
    }
}

// ===========================
// map / map_err / and_then / or_else
// ===========================

[test]
def test_map(t : T?) {
    t |> run("map transforms ok") @(t : T?) {
        let m = ok(3, type<string>) |> map($(x : int) => x * 2)
        t |> equal(m._value, 6)
    }

    t |> run("map leaves err as err") @(t : T?) {
        let m = err("boom", type<int>) |> map($(x : int) => x * 2)
        t |> success(!m._is_ok)
        t |> equal(m._error, "boom")
    }

    t |> run("map can change ok type") @(t : T?) {
        let m = ok(7, type<string>) |> map($(x : int) => "{x}")
        t |> equal(m._value, "7")
    }
}

[test]
def test_map_err(t : T?) {
    t |> run("map_err transforms err") @(t : T?) {
        let m = err("boom", type<int>) |> map_err($(e : string) => "wrapped: {e}")
        t |> equal(m._error, "wrapped: boom")
    }

    t |> run("map_err leaves ok unchanged") @(t : T?) {
        let m = ok(3, type<string>) |> map_err($(e : string) => "x")
        t |> equal(m._value, 3)
    }
}

[test]
def test_and_then(t : T?) {
    t |> run("and_then chains ok → ok") @(t : T?) {
        let next = ok(4, type<string>) |> and_then($(x : int) => ok(x + 10, type<string>))
        t |> equal(next._value, 14)
    }

    t |> run("and_then on err short-circuits") @(t : T?) {
        let next = err("boom", type<int>) |> and_then($(x : int) => ok(x + 10, type<string>))
        t |> success(!next._is_ok)
        t |> equal(next._error, "boom")
    }

    t |> run("and_then can return err") @(t : T?) {
        let next = ok(5, type<string>) |> and_then($(x : int) => err("rejected", type<int>))
        t |> success(!next._is_ok)
        t |> equal(next._error, "rejected")
    }
}

[test]
def test_or_else(t : T?) {
    t |> run("or_else passes ok through") @(t : T?) {
        let rec = ok(1, type<string>) |> or_else($(e : string) => ok(99, type<string>))
        t |> equal(rec._value, 1)
    }

    t |> run("or_else recovers on err") @(t : T?) {
        let rec = err("boom", type<int>) |> or_else($(e : string) => ok(99, type<string>))
        t |> equal(rec._value, 99)
    }
}

// ===========================
// unwrap family
// ===========================

[test]
def test_unwrap(t : T?) {
    t |> run("unwrap on ok returns value") @(t : T?) {
        t |> equal(ok(42, type<string>) |> unwrap, 42)
    }

    t |> run("unwrap_or returns default when err") @(t : T?) {
        t |> equal(err("e", type<int>) |> unwrap_or(17), 17)
        t |> equal(ok(5, type<string>) |> unwrap_or(17), 5)
    }

    t |> run("unwrap_or_else computes from error") @(t : T?) {
        t |> equal(err("xx", type<int>) |> unwrap_or_else($(e : string) => length(e)), 2)
    }

    t |> run("unwrap_or_default uses zero value") @(t : T?) {
        t |> equal(err("e", type<int>) |> unwrap_or_default, 0)
        t |> equal(ok(5, type<string>) |> unwrap_or_default, 5)
    }

    t |> run("unwrap_err on err returns error") @(t : T?) {
        t |> equal(err("boom", type<int>) |> unwrap_err, "boom")
    }

    t |> run("move_unwrap on copyable T returns value, flips r to err state") @(t : T?) {
        var r = ok(42, type<string>)
        t |> equal(r |> move_unwrap, 42)
        t |> success(!r._is_ok)
    }

    t |> run("move_unwrap_err on copyable E returns error, flips r to ok state") @(t : T?) {
        var r = err("boom", type<int>)
        t |> equal(r |> move_unwrap_err, "boom")
        t |> success(r._is_ok)
    }
}

// ===========================
// operators
// ===========================

[test]
def test_operators(t : T?) {
    t |> run("?? unwraps ok") @(t : T?) {
        t |> equal(ok(7, type<string>) ?? 0, 7)
    }

    t |> run("?? returns default on err") @(t : T?) {
        t |> equal(err("e", type<int>) ?? 99, 99)
    }

    t |> run("== compares tag + payload") @(t : T?) {
        t |> success(ok(5, type<string>) == ok(5, type<string>))
        t |> success(!(ok(5, type<string>) == ok(6, type<string>)))
        t |> success(err("x", type<int>) == err("x", type<int>))
        t |> success(!(ok(5, type<string>) == err("x", type<int>)))
    }

    t |> run("== compares result with ok payload value") @(t : T?) {
        t |> success(ok(5, type<string>) == 5)
        t |> success(5 == ok(5, type<string>))
        t |> success(!(ok(5, type<string>) == 6))
        t |> success(!(6 == ok(5, type<string>)))
        t |> success(!(err("x", type<int>) == 0))
        t |> success(!(0 == err("x", type<int>)))
    }

    t |> run("!= compares result with ok payload value") @(t : T?) {
        t |> success(ok(5, type<string>) != 6)
        t |> success(6 != ok(5, type<string>))
        t |> success(!(ok(5, type<string>) != 5))
        t |> success(!(5 != ok(5, type<string>)))
        t |> success(err("x", type<int>) != 0)
        t |> success(0 != err("x", type<int>))
    }
}

// ===========================
// side-effect combinators
// ===========================

[test]
def test_if_ok_if_err(t : T?) {
    t |> run("if_ok fires on ok") @(t : T?) {
        var fired = 0
        ok(3, type<string>) |> if_ok() $(x : int) { fired = x; }
        t |> equal(fired, 3)
    }

    t |> run("if_ok is a no-op on err") @(t : T?) {
        var fired = 0
        err("boom", type<int>) |> if_ok() $(x : int) { fired = x; }
        t |> equal(fired, 0)
    }

    t |> run("if_err fires on err") @(t : T?) {
        var fired = ""
        err("boom", type<int>) |> if_err() $(e : string) { fired = e; }
        t |> equal(fired, "boom")
    }

    t |> run("if_err is a no-op on ok") @(t : T?) {
        var fired = ""
        ok(1, type<string>) |> if_err() $(e : string) { fired = e; }
        t |> equal(fired, "")
    }
}

// ===========================
// bridges to Option
// ===========================

[test]
def test_to_option(t : T?) {
    t |> run("to_option on ok is some") @(t : T?) {
        let o = ok(42, type<string>) |> to_option
        t |> success(o._has_value)
        t |> equal(o._value, 42)
    }

    t |> run("to_option on err is none") @(t : T?) {
        let o = err("boom", type<int>) |> to_option
        t |> success(!o._has_value)
    }

    t |> run("err_to_option on err is some") @(t : T?) {
        let o = err("boom", type<int>) |> err_to_option
        t |> success(o._has_value)
        t |> equal(o._value, "boom")
    }

    t |> run("err_to_option on ok is none") @(t : T?) {
        let o = ok(42, type<string>) |> err_to_option
        t |> success(!o._has_value)
    }
}

// ===========================
// chaining smoke test
// ===========================

[test]
def test_chaining(t : T?) {
    t |> run("ok → map → and_then → unwrap_or") @(t : T?) {
        let result = (ok(3, type<string>)
            |> map($(x : int) => x * 2)
            |> and_then($(x : int) => ok(x + 1, type<string>))
            |> unwrap_or(-1))
        t |> equal(result, 7)
    }

    t |> run("err short-circuits, map_err rewrites") @(t : T?) {
        let on_err = (err("boom", type<int>)
            |> map($(x : int) => x * 2)
            |> map_err($(s : string) => "<{s}>"))
        t |> equal(on_err |> unwrap_or(-1), -1)
        t |> equal(on_err._error, "<boom>")
    }
}
