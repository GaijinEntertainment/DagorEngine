options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/option

// ===========================
// constructors + predicates
// ===========================

[test]
def test_constructors(t : T?) {
    t |> run("some carries the value") @(t : T?) {
        let a = some(42)
        t |> equal(a._value, 42)
        t |> success(a._has_value)
    }

    t |> run("none is empty for the named type") @(t : T?) {
        let b = none(type<int>)
        t |> success(!b._has_value)
    }

    t |> run("is_some / is_none") @(t : T?) {
        t |> success(some(1) |> is_some)
        t |> success(!(none(type<int>) |> is_some))
        t |> success(none(type<string>) |> is_none)
        t |> success(!(some("x") |> is_none))
    }

    t |> run("some / none work for different payload types") @(t : T?) {
        t |> success(some(1.5f)._has_value)
        t |> success(some("hi")._has_value)
        t |> success(!none(type<float>)._has_value)
        t |> success(!none(type<string>)._has_value)
    }
}

// ===========================
// map / filter / and_then / or_else / or_value
// ===========================

[test]
def test_map(t : T?) {
    t |> run("map transforms some") @(t : T?) {
        let m = some(3) |> map($(x : int) => x * 2)
        t |> equal(m._value, 6)
        t |> success(m._has_value)
    }

    t |> run("map leaves none as none") @(t : T?) {
        let m = none(type<int>) |> map($(x : int) => x * 2)
        t |> success(!m._has_value)
    }

    t |> run("map can change the payload type") @(t : T?) {
        let m = some(7) |> map($(x : int) => "{x}")
        t |> equal(m._value, "7")
    }
}

[test]
def test_filter(t : T?) {
    t |> run("filter keeps matching some") @(t : T?) {
        let f = some(10) |> filter($(x : int) => x > 5)
        t |> success(f._has_value)
    }

    t |> run("filter drops non-matching some") @(t : T?) {
        let f = some(3) |> filter($(x : int) => x > 5)
        t |> success(!f._has_value)
    }

    t |> run("filter passes none through") @(t : T?) {
        let f = none(type<int>) |> filter($(x : int) => true)
        t |> success(!f._has_value)
    }
}

[test]
def test_and_then(t : T?) {
    t |> run("and_then chains some → some") @(t : T?) {
        let r = some(4) |> and_then($(x : int) => some(x + 10))
        t |> equal(r._value, 14)
    }

    t |> run("and_then on none short-circuits") @(t : T?) {
        let r = none(type<int>) |> and_then($(x : int) => some(x + 10))
        t |> success(!r._has_value)
    }

    t |> run("and_then can return none") @(t : T?) {
        let r = some(5) |> and_then($(x : int) => none(type<int>))
        t |> success(!r._has_value)
    }
}

[test]
def test_or_else(t : T?) {
    t |> run("or_else passes some through") @(t : T?) {
        let r = some(1) |> or_else() { return some(99); }
        t |> equal(r._value, 1)
    }

    t |> run("or_else invokes fallback on none") @(t : T?) {
        let r = none(type<int>) |> or_else() { return some(99); }
        t |> equal(r._value, 99)
    }
}

[test]
def test_or_value(t : T?) {
    t |> run("or_value passes some through") @(t : T?) {
        let r = some(1) |> or_value(99)
        t |> equal(r._value, 1)
    }

    t |> run("or_value substitutes on none") @(t : T?) {
        let r = none(type<int>) |> or_value(99)
        t |> equal(r._value, 99)
    }
}

// ===========================
// unwrap family
// ===========================

[test]
def test_unwrap(t : T?) {
    t |> run("unwrap on some returns value") @(t : T?) {
        t |> equal(some(42) |> unwrap, 42)
    }

    t |> run("unwrap_or returns default when none") @(t : T?) {
        t |> equal(none(type<int>) |> unwrap_or(17), 17)
        t |> equal(some(5) |> unwrap_or(17), 5)
    }

    t |> run("unwrap_or_else calls block when none") @(t : T?) {
        t |> equal(none(type<int>) |> unwrap_or_else() { return 11; }, 11)
        t |> equal(some(5) |> unwrap_or_else() { return 11; }, 5)
    }

    t |> run("unwrap_or_default uses zero value") @(t : T?) {
        t |> equal(none(type<int>) |> unwrap_or_default, 0)
        t |> equal(none(type<string>) |> unwrap_or_default, "")
        t |> equal(some(5) |> unwrap_or_default, 5)
    }

    t |> run("move_unwrap on copyable T returns value, leaves source none") @(t : T?) {
        var o = some(42)
        t |> equal(o |> move_unwrap, 42)
        t |> success(!o._has_value)
    }
}

// ===========================
// operators
// ===========================

[test]
def test_operators(t : T?) {
    t |> run("?? unwraps some") @(t : T?) {
        t |> equal(some(7) ?? 0, 7)
    }

    t |> run("?? returns default on none") @(t : T?) {
        t |> equal(none(type<int>) ?? 99, 99)
    }

    t |> run("== compares structurally") @(t : T?) {
        t |> success(some(5) == some(5))
        t |> success(!(some(5) == some(6)))
        t |> success(none(type<int>) == none(type<int>))
        t |> success(!(some(5) == none(type<int>)))
    }

    t |> run("== compares option with payload value") @(t : T?) {
        t |> success(some(5) == 5)
        t |> success(5 == some(5))
        t |> success(!(some(5) == 6))
        t |> success(!(6 == some(5)))
        t |> success(!(none(type<int>) == 0))
        t |> success(!(0 == none(type<int>)))
    }

    t |> run("!= compares option with payload value") @(t : T?) {
        t |> success(some(5) != 6)
        t |> success(6 != some(5))
        t |> success(!(some(5) != 5))
        t |> success(!(5 != some(5)))
        t |> success(none(type<int>) != 0)
        t |> success(0 != none(type<int>))
    }
}

// ===========================
// side-effect combinators
// ===========================

[test]
def test_if_some_if_none(t : T?) {
    t |> run("if_some fires on some") @(t : T?) {
        var fired = 0
        some(3) |> if_some() $(x : int) { fired = x; }
        t |> equal(fired, 3)
    }

    t |> run("if_some is a no-op on none") @(t : T?) {
        var fired = 0
        none(type<int>) |> if_some() $(x : int) { fired = x; }
        t |> equal(fired, 0)
    }

    t |> run("if_none fires on none") @(t : T?) {
        var fired = 0
        none(type<int>) |> if_none() { fired = 42; }
        t |> equal(fired, 42)
    }

    t |> run("if_none is a no-op on some") @(t : T?) {
        var fired = 0
        some(1) |> if_none() { fired = 42; }
        t |> equal(fired, 0)
    }
}

// ===========================
// zip
// ===========================

[test]
def test_zip(t : T?) {
    t |> run("zip some + some -> some") @(t : T?) {
        let z = zip(some(1), some(2.5f))
        t |> success(z._has_value)
        t |> equal(z._value._0, 1)
        t |> equal(z._value._1, 2.5f)
    }

    t |> run("zip with either none -> none") @(t : T?) {
        t |> success(!zip(none(type<int>), some(1.5f))._has_value)
        t |> success(!zip(some(1), none(type<float>))._has_value)
        t |> success(!zip(none(type<int>), none(type<float>))._has_value)
    }
}

// ===========================
// chaining smoke test — the whole point of the library
// ===========================

[test]
def test_chaining(t : T?) {
    t |> run("some → map → filter → unwrap_or") @(t : T?) {
        let result = (some(3)
            |> map($(x : int) => x * 2)
            |> filter($(x : int) => x > 0)
            |> unwrap_or(-1))
        t |> equal(result, 6)
    }

    t |> run("none short-circuits the whole chain") @(t : T?) {
        let result = (none(type<int>)
            |> map($(x : int) => x * 2)
            |> filter($(x : int) => x > 0)
            |> unwrap_or(-1))
        t |> equal(result, -1)
    }
}
