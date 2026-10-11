options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

typedef FaFn = function<(a, b : int) : int>

def fa_add(a, b : int) : int {
    return a + b
}

def fa_mul(a, b : int) : int {
    return a * b
}

struct FaSlot {
    handle : uint64
}

def fa_handle_of(f : FaFn) : uint64 {
    return intptr(f)
}

def fa_inline_handle : uint64 {
    return unsafe(reinterpret<uint64>(@@fa_add))
}

[test]
def test_func_addr(t : T?) {
    t |> run("function pointer invoke") @(t : T?) {
        let fn = @@fa_add
        let val = invoke(fn, 1, 2)
        t |> equal(val, 3)
    }
    t |> run("function pointer comparison") @(t : T?) {
        let fn = @@fa_add
        t |> equal(fn != null, true)
        t |> equal(fn == @@fa_add, true)
        t |> equal(fn == null, false)
        t |> equal(fn != @@fa_add, false)
    }
    t |> run("null function pointer") @(t : T?) {
        let qq : function
        t |> equal(qq == null, true)
    }
    t |> run("a function address is a non-null uint64 handle in every store position") @(t : T?) {
        // not its own baseline - the direct spelling is what this block tests
        let handle_via_param = fa_handle_of(@@fa_add)
        t |> success(handle_via_param != 0ul, "the handle through a parameter is not null")
        unsafe {
            var assigned : uint64
            assigned = reinterpret<uint64>(@@fa_add)   // nolint:STYLE011 - the split IS the case under test: a store into a live local, not an initializer
            let declared = reinterpret<uint64>(@@fa_add)
            var fixarr : uint64[2]
            fixarr[1] = reinterpret<uint64>(@@fa_add)
            var slot : FaSlot
            slot.handle = reinterpret<uint64>(@@fa_add)
            let literal = FaSlot(handle = reinterpret<uint64>(@@fa_add))
            let arith = 0ul + reinterpret<uint64>(@@fa_add)
            let as_ptr = reinterpret<void?>(@@fa_add)
            t |> equal(assigned, handle_via_param, "assignment to a local")
            t |> equal(declared, handle_via_param, "declaration with initializer")
            t |> equal(fixarr[1], handle_via_param, "fixed-array element store")
            t |> equal(slot.handle, handle_via_param, "struct field assignment")
            t |> equal(literal.handle, handle_via_param, "struct literal field")
            t |> equal(arith, handle_via_param, "arithmetic operand")
            t |> equal(intptr(as_ptr), handle_via_param, "cast to void?")
            t |> equal(reinterpret<int64>(@@fa_add), int64(handle_via_param), "cast to int64")
            t |> equal(invoke(reinterpret<FaFn>(as_ptr), 20, 22), 42, "a void? handle casts back to a callable")
        }
        t |> equal(fa_inline_handle(), handle_via_param, "the direct spelling inside a return")
    }
    t |> run("distinct functions get distinct handles") @(t : T?) {
        t |> success(fa_handle_of(@@fa_add) != fa_handle_of(@@fa_mul),
            "two functions do not share a handle")
    }
    t |> run("a uint64 handle casts back to a callable") @(t : T?) {
        unsafe {
            let h = reinterpret<uint64>(@@fa_add)
            t |> equal(invoke(reinterpret<FaFn>(h), 20, 22), 42)
        }
    }
    t |> run("invoke null function panics") @(t : T?) {
        var failed = false
        try {
            let fn2 : function<(a, b : int) : int>
            invoke(fn2, 1, 2)
        } recover {
            failed = true
        }
        t |> equal(failed, true)
    }
}
