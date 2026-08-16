options gen2
options auto_inline_functions = true

require UnitTest
require dastest/testing_boost

// An argument the inliner cannot substitute (here a ternary, which is neither a ref nor pure) binds a
// `let __inl<N>_arg_<param>` temp instead. The temp is a by-value local, so a parameter type that
// cannot be a local at all - TestObjectNotLocal reports isLocal()==false - produces a declaration
// infer rejects, and the diagnostics name a variable the author never wrote:
//
//   error[31020]: local variable of type TestObjectNotLocal const requires unsafe
//   error[30199]: local variable __inl0_arg_o can only be initialized with type constructor
//
// The inliner already refuses a result type that cannot take the manufactured-temp path; the
// argument temps need the same check, and must leave the call alone instead of emitting a local
// whose internal name reaches the user.

def read_data(o : TestObjectNotLocal) : int {
    return o.fooData
}

def pick_not_local(c : bool) : int {
    var total = 0
    testNotLocalObject() $(o) {
        let p = unsafe(addr(o))
        total = read_data(c ? o : *p)
    }
    return total
}

[test]
def test_inline_arg_temp_of_non_local_type(t : T?) {
    t |> equal(pick_not_local(true), 13)
    t |> equal(pick_not_local(false), 13)
}
