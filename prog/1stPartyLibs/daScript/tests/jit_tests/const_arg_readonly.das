options gen2
options _nolint = "LINT031"   // the write through a const-stripped parameter is the shape under test

require dastest/testing_boost
require _const_arg_readonly_writer

// A const pointer or reference parameter the callee writes through after an unsafe cast.
// The JIT gives a const indirect parameter the LLVM readonly attribute unless the compiler
// marked the parameter written; with the attribute in place the optimizer folds the caller's
// read of that memory to its pre-call value from --jit-opt-level=1 up. Every check passes in
// the interpreter and at --jit-opt-level=0 whatever the attribute says.

struct Cell {
    a : int
    b : int
}

// const void? out-parameter, cast to a typed pointer
[hint(noinline)]
def route_void(dst : void?; v : int) {
    var p = unsafe(reinterpret<int?>(dst))
    *p = v
}

// const typed pointer, cast to its own type
[hint(noinline)]
def route_same_type(dst : int?; v : int) {
    var p = unsafe(reinterpret<int?>(dst))
    *p = v
}

// const void? out-parameter as a memcpy destination
[hint(noinline)]
def route_memcpy(dst : void?; v : int) {
    var src = v
    unsafe {
        memcpy(dst, addr<void?>(src), 4)
    }
}

// const structure reference, address taken and cast back
[hint(noinline)]
def route_const_ref(c : Cell; v : int) {
    var p = unsafe(addr<Cell?>(c))
    p.a = v
}

// const typed pointer under a noalias hint - with no readonly the hint alone keeps the write
[hint(noinline, noalias = dst)]
def route_noalias(dst : int const?; v : int) {
    var p = unsafe(reinterpret<int?>(dst))
    *p = v
}

[test]
def test_write_through_const_argument(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@route_void))
    t |> run("const typed pointer under noalias") @(t : T?) {
        var slot = 0
        route_noalias(unsafe(addr(slot)), 40)
        t |> equal(40, slot)
    }
    t |> run("const void? out-parameter") @(t : T?) {
        var slot = 0
        route_void(unsafe(addr<void?>(slot)), 41)
        t |> equal(41, slot)
    }
    t |> run("const typed pointer") @(t : T?) {
        var slot = 0
        route_same_type(unsafe(addr(slot)), 42)
        t |> equal(42, slot)
    }
    t |> run("const void? memcpy destination") @(t : T?) {
        var slot = 0
        route_memcpy(unsafe(addr<void?>(slot)), 43)
        t |> equal(43, slot)
    }
    t |> run("const structure reference") @(t : T?) {
        var cell = Cell(a = 0, b = 0)
        route_const_ref(cell, 44)
        t |> equal(44, cell.a)
    }
}

// the control: the same body with a var parameter carries no readonly and is correct in
// every mode
[hint(noinline)]
def route_var_parameter(var dst : void?; v : int) {
    var p = unsafe(reinterpret<int?>(dst))
    *p = v
}

[test]
def test_var_parameter_is_the_control(t : T?) {
    var slot = 0
    route_var_parameter(unsafe(addr<void?>(slot)), 47)
    t |> equal(47, slot, "the same body with a var parameter")
}

[test]
def test_write_through_const_argument_across_modules(t : T?) {
    var slot = 0
    store_int(unsafe(addr<void?>(slot)), 46)
    t |> equal(46, slot, "a cross-partition callee needs no inlining hint")
}
