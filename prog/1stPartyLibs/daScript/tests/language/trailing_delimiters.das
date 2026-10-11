options gen2
require dastest/testing_boost public

// Tests trailing delimiter syntax (trailing commas) in various contexts:
// arrays, struct initializers, table literals, blocks, mixed expressions.

[safe_when_uninitialized]
struct AAA {
    x : int = 1
    y : int = 1
}

[test]
def test_trailing_delimiters(t : T?) {  // nolint:STYLE038 - flat list of independent test arms
    t |> run("int array literals") @(t : T?) {
        let i0 <- fixed_array(1, 2, 1)
        t |> equal(i0[0], 1)
        t |> equal(i0[1], 2)
        let i1 <- fixed_array<int>(1, 2, 1)
        t |> equal(i1[0], 1)
        let i2 <- fixed_array<int>(1, 2, 1)
        t |> equal(i2[2], 1)
        let i3 <- [1, 2]
        t |> equal(length(i3), 2)
        let i4 <- [1]
        t |> equal(length(i4), 1)
        let i5 <- [1]
        t |> equal(i5[0], 1)
        // nested indexing
        let x = i0[i1[i2[0]]]
        t |> equal(x, 1)
    }
    t |> run("structure initializer trailing commas") @(t : T?) {
        let a <- [
            AAA(x=1, y=2),
            AAA(x=3, y=4),
        ]
        t |> equal(a[0].x, 1)
        t |> equal(a[1].x, 3)
        let b <- [
            AAA(x=1, y=2),
            AAA(x=3, y=4),
        ]
        t |> equal(b[0].y, 2)
        t |> equal(b[1].y, 4)
        let ad <- [AAA(
            x = 1, y = 2), AAA(
            x = 3, y = 4)]
        t |> equal(ad[0].x, 1)
        t |> equal(ad[1].x, 3)
        let c <- AAA(
            x = 1,
            y = 2)
        t |> equal(c.x, 1)
        t |> equal(c.y, 2)
        let e <- [AAA(x=111, y=222), AAA(x=333, y=444)]
        t |> equal(e[1].y, 444)
        let f <- [AAA(x=111, y=222), AAA(x=333, y=444)]
        t |> equal(f[1].y, 444)
    }
    t |> run("table literal trailing commas") @(t : T?) {
        let t0 <- {
            1 => 111,
            2 => 222}
        t |> equal(t0?[1] ?? 0, 111)
        t |> equal(t0?[2] ?? 0, 222)
    }
    t |> run("block syntax") @(t : T?) {
        // various block forms should compile without errors
        ${}
        ${
        }
        ${; }
        ${ ${}; }
        ${ ${}; }
        ${ ${}; }
        ${ ${ ${}; }
        }
        ${feint(""); }
        {feint(""); }
    }
    t |> run("mixed blocks with lambdas") @(t : T?) {
        let a0 <- { 1 => @(a : string) { feint(a); } }
        t |> equal(length(a0), 1)
        let a2 <- { 1 => @(a : string) { let a3 <- { 1 => @(aa : string) { feint(aa); } }; } }
        t |> equal(length(a2), 1)
    }
    t |> run("copy move return") @(t : T?) {
        // Test copy-on-return with struct
        let tr = addBar_helper(newFoo_helper(), newFoo_helper())
        t |> equal(tr, 2)
        // Test move-on-return with arrays
        let z <- joinSeq_helper(makeSeq_helper(0, 4), makeSeq_helper(5, 9))
        t |> equal(length(z), 10)
        for (i in range(10)) {
            t |> equal(z[i], i)
        }
    }
}

// Helpers for copy_and_move_on_return tests

struct FooCMR {
    bar : int
}

def newFoo_helper() {
    var f : FooCMR
    f.bar = 1
    return f
}

def addBar_helper(a, b : FooCMR) {
    return a.bar + b.bar
}

def makeSeq_helper(var base : int; max : int) {
    var a : array<int>
    resize(a, max + 1 - base)
    for (el in a) {
        el = base++
    }
    return <- a
}

def joinSeq_helper(l : array<int>; r : array<int>) {
    var a : array<int>
    reserve(a, length(l) + length(r))
    a |> push_from(l, r)
    return <- a
}
