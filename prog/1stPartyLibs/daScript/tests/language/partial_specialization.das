// Tests generic function partial specialization: cast distance, auto, dim, containers, blocks
options gen2
require dastest/testing_boost public

let allow_broken = false

struct Foo {
    a : int
}

struct Bar : Foo {
    b : int
}

struct Far : Bar {
    c : int
}

def test(a : Foo; b : Foo) { return "foo foo"; }
def test(a : Foo; b : Bar) { return "foo bar"; }
def test(a : Bar; b : Foo) { return "bar foo"; }

def test_distance(t : T?) {
    t |> equal(test(Foo(uninitialized), Foo(uninitialized)), "foo foo")
    t |> equal(test(Foo(uninitialized), Bar(uninitialized)), "foo bar")
    t |> equal(test(Bar(uninitialized), Foo(uninitialized)), "bar foo")
    t |> equal(test(Far(uninitialized), Bar(uninitialized)), "foo bar")
    static_if (allow_broken) {
        test(Bar(uninitialized), Bar(uninitialized))
    }
}

def gen_1(a : Foo; b) { return "Foo *"; }
def gen_1(a; b) { return "* *"; }

def gen_2(a : Foo; b) { return "Foo *"; }
def gen_2(a : Bar; b) { return "Bar *"; }

def gen_3(arr) { return "*"; }
def gen_3(arr : auto[]) { return "[*]"; }
def gen_3(arr : auto[3]) { return "[3]"; }

def gen_4(arr) { return "*"; }
def gen_4(arr : array<auto>) { return "array<*>"; }

def gen_5(arr : array<auto?>) { return "<*?>"; }
def gen_5(arr : array<auto>) { return "<*>"; }

def gen_6(arr) { return "*"; }
def gen_6(arr : table<auto; auto?>) { return "<*:*?>"; }
def gen_6(arr : table<int; auto>) { return "<int:*>"; }
def gen_6(arr : table<int; auto?>) { return "<int:*?>"; }

def gen_7(arr : tuple<auto; auto?; int>) { return "<*:*?:int>"; }
def gen_7(arr : tuple<auto; auto; auto>) { return "<*:*:*>"; }

def gen_8(arr : auto) { return "*"; }
def gen_8(arr : function<(arg : auto; arg2 : int) : auto?>) { return "(*:int):?"; }
def gen_8(arr : function<(arg : auto; arg2 : int) : auto>) { return "(*:int):*"; }

[test]
def test_partial_specialization(t : T?) {
    t |> run("cast distance") @(t : T?) {
        test_distance(t)
    }
    t |> run("non-auto more specialized than auto") @(t : T?) {
        t |> equal(gen_1(Foo(uninitialized), Foo(uninitialized)), "Foo *")
    }
    t |> run("exact type more specialized than base") @(t : T?) {
        t |> equal(gen_2(Bar(uninitialized), Foo(uninitialized)), "Bar *")
    }
    t |> run("dim specialization") @(t : T?) {
        t |> equal(gen_3(fixed_array<int>(1, 2, 3, 4)), "[*]")
        t |> equal(gen_3(fixed_array<int>(1, 2, 3)), "[3]")
    }
    t |> run("container specialization") @(t : T?) {
        let ta : array<int>
        t |> equal(gen_4(ta), "array<*>")
    }
    t |> run("pointer subtype specialization") @(t : T?) {
        let tp : array<int?>
        t |> equal(gen_5(tp), "<*?>")
    }
    t |> run("table multi-subtype specialization") @(t : T?) {
        let tt : table<int; int?>
        t |> equal(gen_6(tt), "<int:*?>")
        let tf : table<float; int?>
        t |> equal(gen_6(tf), "<*:*?>")
    }
    t |> run("tuple subtype specialization") @(t : T?) {
        let tu : tuple<float; int?; int>
        t |> equal(gen_7(tu), "<*:*?:int>")
    }
    t |> run("function signature specialization") @(t : T?) {
        let fn : function<(arg : float; arg2 : int) : void?>
        t |> equal(gen_8(fn), "(*:int):?")
    }
}
