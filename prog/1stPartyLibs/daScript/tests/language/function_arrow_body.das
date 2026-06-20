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

// arrow body on struct/class methods (the member-function form).
// `tag` carries a field annotation immediately after an arrow-bodied method:
// this is the case that previously broke the parser (the '@' was ambiguous with
// a trailing-block pipe), now disambiguated at the lexer.
class Widget {
    w : int
    h : int
    def area() : int => w * h
    @do_not_delete tag : int
    def perimeter() : int => 2 * (w + h)
    def scaled(k : int) : int => area() * k
    def static unit() : int => 1
}

struct Vec2 {
    x : int
    y : int
    def dot(o : Vec2) : int => x * o.x + y * o.y
    def cloned() : Vec2 => <- Vec2(x = x, y = y)
}

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
    t |> run("class method arrow bodies") @(t : T?) {
        var wp = new Widget(w = 3, h = 4)
        t |> equal(wp->area(), 12)
        t |> equal(wp->perimeter(), 14)
        t |> equal(wp->scaled(2), 24)
        t |> equal(Widget`unit(), 1)
        unsafe { delete wp; }
    }
    t |> run("struct method arrow bodies") @(t : T?) {
        var a = Vec2(x = 1, y = 2)
        var b = Vec2(x = 3, y = 4)
        t |> equal(a.dot(b), 11)
        let c <- a.cloned()
        t |> equal(c.x, 1)
        t |> equal(c.y, 2)
    }
}
