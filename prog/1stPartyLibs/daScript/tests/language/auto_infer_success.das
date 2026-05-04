// Tests successful auto type inference: globals, arrays, pointers, tables, blocks, generics, tuples
options gen2
options no_unused_block_arguments = true
options no_unused_function_arguments = false
require dastest/testing_boost public

struct Foo {
    bar : int
}

let intnum = 5
let floatnum = 5.5
let stringval = "hello, world"

let some_a = "test"
let some_b <- [some_a]

def foo(b : block<(a : int) : int>) {
    return invoke(b, 3)
}

def foobar(b : block<(a : int; b : float) : string>) {
    return invoke(b, 1, 2.0) + "bar"
}

def madd(a, b, c) {
    return a * b + c
}

def take_arr(a : int[]) {
    var summ = 0
    for (x in a) {
        summ += x
    }
    return summ
}

def random_pick(source : string[]; var seed : int4&) : string {
    return "pick_1"
}

def random_pick(source : tuple<string; float>[]; var seed : int4&) : string {
    return "pick_2"
}

[sideeffects]
def qq(a : auto(T1); b : block<(a1 : T1; a2 : T2) : bool>; c : auto(T2)) {
    pass
}

def fn3 { return 3; }
def fn2 { return fn3() + 1; }
def fn1 { return fn2() + 2; }

[test]
def test_auto_infer(t : T?) {
    t |> run("global infer") @(t : T?) {
        t |> equal(length(some_b), 1)
    }
    t |> run("fixed array copy and const") @(t : T?) {
        let a : int[10]
        let b = a
        let c : auto const = b
        t |> equal(length(a), 10)
    }
    t |> run("array size deduction") @(t : T?) {
        t |> equal(take_arr(fixed_array<int>(1, 2, 3, 4)), 10)
    }
    t |> run("pointer auto-infer") @(t : T?) {
        var pFoo : Foo? = new Foo
        var pFooI : auto? = pFoo
        pFooI.bar = 1
        pFoo = pFooI
        t |> equal(pFoo.bar, 1)
    }
    t |> run("container auto-infer") @(t : T?) {
        var TT_A : array<int>
        let TT_iA : array<auto> <- TT_A
        var TT_T : table<string; int>
        let TT_iT : table<auto; auto> <- TT_T
        let TT_ikT : table<auto; int> <- TT_T
        let TT_ivT : table<string; auto> <- TT_T
    }
    t |> run("block return type infer") @(t : T?) {
        let f1 = foo() <| $(arg : int) {
            return arg + 1
        }
        t |> equal(f1, 4)
    }
    t |> run("block argument default infer") @(t : T?) {
        let f2 = foo() <| $(arg = 5) {
            return arg + 2
        }
        t |> equal(f2, 5)
    }
    t |> run("block full forward infer") @(t : T?) {
        let f3 = foo() <| $(arg) {
            return arg + 3
        }
        t |> equal(f3, 6)
    }
    t |> run("block two arguments infer") @(t : T?) {
        let blah = foobar() <| $(ia, fb) {
            t |> equal(ia, 1)
            t |> equal(fb, 2.0)
            return "foo"
        }
        t |> equal(blah, "foobar")
    }
    t |> run("function return type infer") @(t : T?) {
        t |> equal(fn1(), 6)
    }
    t |> run("generic type deduction") @(t : T?) {
        t |> equal(madd(1, 2, 3), 5)
        t |> equal(madd(1.0, 2.0, 3.0), 5.0)
    }
    t |> run("tuple auto-infer") @(t : T?) {
        let ZZT = ("two", 2)
        t |> equal(typeinfo is_dim(ZZT), false)
        let ZZA = fixed_array(("two", 2))
        t |> equal(typeinfo is_dim(ZZA), true)
        t |> equal(typeinfo dim(ZZA), 1)
    }
    t |> run("overload by dim type") @(t : T?) {
        let s1 : string[2]
        let s2 : tuple<string; float>[3]
        var seed : int4
        t |> equal(random_pick(s1, seed), "pick_1")
        t |> equal(random_pick(s2, seed), "pick_2")
    }
    t |> run("block generic with alias") @(t : T?) {
        qq(1, $[unused_argument(XX, YY)] (XX, YY) => true, 2.0)
    }
}
