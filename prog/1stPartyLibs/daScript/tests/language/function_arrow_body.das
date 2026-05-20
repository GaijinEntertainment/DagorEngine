// Locks the gen2 grammar production: `def name(args) : T => expr` and `def name(args) : T => <- expr`
options gen2
require dastest/testing_boost public

def add(a, b : int) : int => a + b

def public greet(name : string) : string => "hello, {name}"

def private secret(x : int) : int => x * 7

def make_arr() : array<int> => <- [1, 2, 3]

[generic]
def dbl(x : auto(T)) : auto(T) => x + x

def no_ret(x : int) => x + 1

[test]
def test_function_arrow_body(t : T?) {
    t |> run("basic value arrow") @(t : T?) {
        t |> equal(add(2, 3), 5)
        t |> equal(add(-1, 1), 0)
    }
    t |> run("public visibility prefix") @(t : T?) {
        t |> equal(greet("world"), "hello, world")
    }
    t |> run("private visibility prefix") @(t : T?) {
        t |> equal(secret(3), 21)
    }
    t |> run("move-arrow returning array") @(t : T?) {
        let xs <- make_arr()
        t |> equal(length(xs), 3)
        t |> equal(xs[0], 1)
        t |> equal(xs[2], 3)
    }
    t |> run("generic with [generic] annotation") @(t : T?) {
        t |> equal(dbl(5), 10)
        t |> equal(dbl(2.5), 5.0)
    }
    t |> run("no explicit return type") @(t : T?) {
        t |> equal(no_ret(10), 11)
    }
}
