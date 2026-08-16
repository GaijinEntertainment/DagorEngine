options gen2
require dastest/testing_boost

require UnitTest

// regression for issue #2606: JIT codegen crashed on a move-return of a
// register-sized handled value type. the make-expression carries useCMRES while
// the enclosing function returns in a register (non-cmres), so there is no cmres
// param to write into — JIT used to read a non-existent parameter and segfault.
// the function must be [export] so it gets a register-return wrapper (the
// register-return ABI is what makes it non-cmres and triggers the bug).
[export]
def make_big : BigEntityId {
    return <- BigEntityId()
}

// consume the result without string-interpolating it (interpolating a handled
// value type walks it, which is an unrelated path).
def sink(_b : BigEntityId) : int {
    return 1
}

[test]
def test_handle_value_move_return(t : T?) {
    t |> run("move-return register-sized handled value") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_big))
        let y = make_big()
        t |> equal(sink(y), 1)
    }
    // reading the moved-in value must match a freshly constructed one. before the
    // fix, the register-returned value was a bogus pointer, so this crashed.
    t |> run("move-returned value is readable") @(t : T?) {
        let y = make_big()
        t |> equal("{y}", "{BigEntityId()}")
    }
}
