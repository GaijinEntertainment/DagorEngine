options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/result
require strings

// Result<T, E> with a non-copyable T (array<int>).
//
// These tests exercise every public function in result.das with a
// non-copyable value-side payload. The error side (E = string) stays
// copyable, so the failures pinpoint the value-side copy/return paths
// that still need migration to the static_if (typeinfo can_copy(...)) +
// clone_to_move(...) pattern already applied in option.das.

def operator ==(a : array<int>; b : array<int>) : bool {
    if (length(a) != length(b)) {
        return false
    }
    for (i in 0 .. length(a)) {
        if (a[i] != b[i]) {
            return false
        }
    }
    return true
}

def make_arr(a : int; b : int) : array<int> {
    return <- [a, b]
}

[test]
def test_constructors(t : T?) {
    t |> run("ok carries a non-copyable payload") @(t : T?) {
        let r = ok(make_arr(1, 2), type<string>)
        t |> success(r._is_ok)
        t |> equal(length(r._value), 2)
        t |> equal(r._value[0], 1)
        t |> equal(r._value[1], 2)
    }

    t |> run("err with a non-copyable T witness") @(t : T?) {
        let r = err("boom", type<array<int>>)
        t |> success(!r._is_ok)
        t |> equal(r._error, "boom")
    }

    t |> run("is_ok / is_err on Result<array<int>, string>") @(t : T?) {
        t |> success(ok(make_arr(1, 2), type<string>) |> is_ok)
        t |> success(err("e", type<array<int>>) |> is_err)
    }
}

[test]
def test_move_ok(t : T?) {
    t |> run("move_ok moves a non-copyable value payload out of the source") @(t : T?) {
        var src = make_arr(1, 2)
        let r = move_ok(src, type<string>)
        t |> success(r._is_ok)
        t |> equal(length(r._value), 2)
        t |> equal(r._value[0], 1)
        t |> equal(r._value[1], 2)
        t |> equal(length(src), 0)
    }

    t |> run("move_ok works on a copyable payload too") @(t : T?) {
        let n = 42
        let r = move_ok(n, type<string>)
        t |> success(r._is_ok)
        t |> equal(r._value, 42)
    }
}

[test]
def test_move_err(t : T?) {
    t |> run("move_err moves a non-copyable error payload out of the source") @(t : T?) {
        var src = make_arr(7, 8)
        let r = move_err(src, type<int>)
        t |> success(!r._is_ok)
        t |> equal(length(r._error), 2)
        t |> equal(r._error[0], 7)
        t |> equal(r._error[1], 8)
        t |> equal(length(src), 0)
    }

    t |> run("move_err works on a copyable error too") @(t : T?) {
        let s = "boom"
        let r = move_err(s, type<int>)
        t |> success(!r._is_ok)
        t |> equal(r._error, "boom")
    }
}

[test]
def test_map(t : T?) {
    t |> run("map Result<array<int>, string> -> Result<int, string>") @(t : T?) {
        let m = ok(make_arr(1, 2), type<string>) |> map($(v : array<int>) => length(v))
        t |> success(m._is_ok)
        t |> equal(m._value, 2)
    }

    t |> run("map keeps a non-copyable output type") @(t : T?) {
        let m = ok(make_arr(1, 2), type<string>) |> map($(v : array<int>) => make_arr(length(v), 0))
        t |> success(m._is_ok)
        t |> equal(m._value[0], 2)
    }

    t |> run("map leaves err as err") @(t : T?) {
        let m = err("boom", type<array<int>>) |> map($(v : array<int>) => length(v))
        t |> success(!m._is_ok)
        t |> equal(m._error, "boom")
    }
}

[test]
def test_map_err(t : T?) {
    t |> run("map_err leaves ok unchanged") @(t : T?) {
        let m = ok(make_arr(1, 2), type<string>) |> map_err($(e : string) => "<{e}>")
        t |> success(m._is_ok)
        t |> equal(length(m._value), 2)
        t |> equal(m._value[0], 1)
    }

    t |> run("map_err transforms err side") @(t : T?) {
        let m = err("boom", type<array<int>>) |> map_err($(e : string) => "<{e}>")
        t |> success(!m._is_ok)
        t |> equal(m._error, "<boom>")
    }
}

[test]
def test_and_then(t : T?) {
    t |> run("and_then chains ok -> ok") @(t : T?) {
        let r = ok(make_arr(1, 2), type<string>) |> and_then($(v : array<int>) => ok(length(v), type<string>))
        t |> equal(r._value, 2)
    }

    t |> run("and_then with non-copyable output") @(t : T?) {
        let r = ok(make_arr(1, 2), type<string>) |> and_then($(v : array<int>) => ok(make_arr(length(v), 0), type<string>))
        t |> success(r._is_ok)
        t |> equal(r._value[0], 2)
    }

    t |> run("and_then on err short-circuits") @(t : T?) {
        let r = err("boom", type<array<int>>) |> and_then($(v : array<int>) => ok(length(v), type<string>))
        t |> success(!r._is_ok)
        t |> equal(r._error, "boom")
    }
}

[test]
def test_or_else(t : T?) {
    t |> run("or_else passes ok through") @(t : T?) {
        let r = ok(make_arr(1, 2), type<string>) |> or_else($(e : string) => ok(make_arr(9, 9), type<string>))
        t |> success(r._is_ok)
        t |> equal(r._value[0], 1)
    }

    t |> run("or_else recovers from err") @(t : T?) {
        let r = err("boom", type<array<int>>) |> or_else($(e : string) => ok(make_arr(9, 10), type<string>))
        t |> success(r._is_ok)
        t |> equal(r._value[0], 9)
    }
}

[test]
def test_unwrap(t : T?) {
    t |> run("unwrap on ok returns the array") @(t : T?) {
        let arr = ok(make_arr(5, 6), type<string>) |> unwrap
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 5)
    }

    t |> run("unwrap_or returns fallback on err") @(t : T?) {
        let arr = err("e", type<array<int>>) |> unwrap_or(make_arr(9, 10))
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 9)
    }

    t |> run("unwrap_or_default returns empty on err") @(t : T?) {
        let arr = err("e", type<array<int>>) |> unwrap_or_default
        t |> equal(length(arr), 0)
    }

    t |> run("unwrap_or_else computes fallback from error") @(t : T?) {
        let arr = err("xx", type<array<int>>) |> unwrap_or_else($(e : string) => make_arr(length(e), 0))
        t |> equal(arr[0], 2)
    }

    t |> run("unwrap_err returns the error") @(t : T?) {
        let e = err("boom", type<array<int>>) |> unwrap_err
        t |> equal(e, "boom")
    }
}

[test]
def test_move_unwrap(t : T?) {
    t |> run("move_unwrap moves the array out of ok, flips r to err state") @(t : T?) {
        var r = ok(make_arr(5, 6), type<string>)
        let arr <- r |> move_unwrap
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 5)
        t |> equal(arr[1], 6)
        // Source is moved-from: tag flipped, payload zeroed.
        t |> success(!r._is_ok)
        t |> equal(length(r._value), 0)
    }
}

[test]
def test_expect(t : T?) {
    t |> run("expect_value on ok returns the array") @(t : T?) {
        let arr = ok(make_arr(7, 8), type<string>) |> expect_value("never fires")
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 7)
    }

    t |> run("expect_err on err returns the error") @(t : T?) {
        let e = err("boom", type<array<int>>) |> expect_err("never fires")
        t |> equal(e, "boom")
    }
}

[test]
def test_if_ok_if_err(t : T?) {
    t |> run("if_ok fires with the array") @(t : T?) {
        var seen = 0
        ok(make_arr(3, 4), type<string>) |> if_ok() $(v : array<int>) { seen = length(v); }
        t |> equal(seen, 2)
    }

    t |> run("if_ok is a no-op on err") @(t : T?) {
        var seen = 0
        err("boom", type<array<int>>) |> if_ok() $(v : array<int>) { seen = length(v); }
        t |> equal(seen, 0)
    }

    t |> run("if_err fires with the error") @(t : T?) {
        var seen = ""
        err("boom", type<array<int>>) |> if_err() $(e : string) { seen = e; }
        t |> equal(seen, "boom")
    }

    t |> run("if_err is a no-op on ok") @(t : T?) {
        var seen = ""
        ok(make_arr(1, 2), type<string>) |> if_err() $(e : string) { seen = e; }
        t |> equal(seen, "")
    }
}

[test]
def test_bridges(t : T?) {
    t |> run("to_option of ok carries the array") @(t : T?) {
        let o = ok(make_arr(7, 8), type<string>) |> to_option
        t |> success(o._has_value)
        t |> equal(length(o._value), 2)
    }

    t |> run("to_option of err is none") @(t : T?) {
        let o = err("e", type<array<int>>) |> to_option
        t |> success(!o._has_value)
    }

    t |> run("err_to_option of err carries the error") @(t : T?) {
        let o = err("boom", type<array<int>>) |> err_to_option
        t |> success(o._has_value)
        t |> equal(o._value, "boom")
    }

    t |> run("err_to_option of ok is none") @(t : T?) {
        let o = ok(make_arr(1, 2), type<string>) |> err_to_option
        t |> success(!o._has_value)
    }
}

[test]
def test_op_qq(t : T?) {
    t |> run("?? unwraps ok array") @(t : T?) {
        let arr = ok(make_arr(7, 8), type<string>) ?? make_arr(0, 0)
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 7)
    }

    t |> run("?? returns fallback on err") @(t : T?) {
        let arr = err("e", type<array<int>>) ?? make_arr(9, 10)
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 9)
    }
}

[test]
def test_equality(t : T?) {
    t |> run("== on Result<array<int>, string>") @(t : T?) {
        t |> success(ok(make_arr(1, 2), type<string>) == ok(make_arr(1, 2), type<string>))
        t |> success(!(ok(make_arr(1, 2), type<string>) == ok(make_arr(1, 3), type<string>)))
        t |> success(err("e", type<array<int>>) == err("e", type<array<int>>))
        t |> success(!(ok(make_arr(1, 2), type<string>) == err("e", type<array<int>>)))
    }
}

[test]
def test_op_eq_value(t : T?) {
    t |> run("Result<array<int>, string> == array<int>") @(t : T?) {
        t |> success(ok(make_arr(1, 2), type<string>) == make_arr(1, 2))
        t |> success(make_arr(1, 2) == ok(make_arr(1, 2), type<string>))
        t |> success(!(ok(make_arr(1, 2), type<string>) == make_arr(1, 3)))
        t |> success(!(err("e", type<array<int>>) == make_arr(1, 2)))
    }

    t |> run("Result<array<int>, string> != array<int>") @(t : T?) {
        t |> success(ok(make_arr(1, 2), type<string>) != make_arr(1, 3))
        t |> success(make_arr(1, 3) != ok(make_arr(1, 2), type<string>))
        t |> success(err("e", type<array<int>>) != make_arr(1, 2))
    }
}

[test]
def test_chaining(t : T?) {
    t |> run("ok -> map -> and_then -> unwrap_or") @(t : T?) {
        let n = (ok(make_arr(1, 2), type<string>)
            |> map($(v : array<int>) => length(v))
            |> and_then($(x : int) => ok(x + 1, type<string>))
            |> unwrap_or(-1))
        t |> equal(n, 3)
    }

    t |> run("err short-circuits the whole chain") @(t : T?) {
        let n = (err("boom", type<array<int>>)
            |> map($(v : array<int>) => length(v))
            |> and_then($(x : int) => ok(x + 1, type<string>))
            |> unwrap_or(-1))
        t |> equal(n, -1)
    }
}
