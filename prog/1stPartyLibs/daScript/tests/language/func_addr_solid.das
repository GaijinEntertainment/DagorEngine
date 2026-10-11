options gen2
options solid_context
options indenting = 4

require dastest/testing_boost public

// Under `options solid_context` a function address simulates as an index into the context's
// function table rather than a mangled-name lookup, which is a different node with its own
// typed reads. The handle has to come out the same in every store position here too.

typedef SolidFn = function<(a, b : int) : int>

def sa_add(a, b : int) : int {
    return a + b
}

def sa_handle_of(f : SolidFn) : uint64 {
    return intptr(f)
}

[test]
def test_solid_context_function_address(t : T?) {
    let handle_via_param = sa_handle_of(@@sa_add)
    t |> success(handle_via_param != 0ul, "the handle through a parameter is not null")
    unsafe {
        let declared = reinterpret<uint64>(@@sa_add)
        let as_int64 = reinterpret<int64>(@@sa_add)
        let as_ptr = reinterpret<void?>(@@sa_add)
        t |> equal(declared, handle_via_param, "declaration with initializer")
        t |> equal(as_int64, int64(handle_via_param), "cast to int64")
        t |> equal(intptr(as_ptr), handle_via_param, "cast to void?")
        t |> equal(invoke(reinterpret<SolidFn>(declared), 20, 22), 42, "a handle casts back to a callable")
    }
}
