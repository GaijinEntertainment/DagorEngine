options gen2
require dastest/testing_boost public

require _operators_derived

struct Foo {
    a : int
}

struct Bar {
    a : float
}

def operator as Bar(a : Foo) {
    return Bar(a = float(a.a))
}

def operator ?as Bar(a : Foo?) : Bar? {
    if (a == null) {
        return null
    }
    return new Bar(a = float(a.a))
}

def operator is Bar(anything) {
    return false
}

def operator is Bar(_a : Bar) {
    return true
}

struct Goo {
    a : string
}

def operator [] (t : Goo; cmp : string) {
    return "{t . . a}-{cmp}"
}

def operator ?[] (t : Goo; cmp : string) : string const?# {
    if (cmp == t . . a) {
        unsafe {
            return addr<string const?#>(t . . a)
        }
    } else {
        unsafe {
            return reinterpret<string const?#>(null)
        }
    }
}

def operator .(t : Goo; name : string) {
    return Goo(a = "{t . . a}.{name}")
}

def operator ?.(t : Goo; name : string) : string const?# {
    if (name == "yes") {
        unsafe {
            return addr<string const?#>(t . . a)
        }
    } else {
        unsafe {
            return reinterpret<string const?#>(null)
        }
    }
}

def operator . length(t : Goo) {
    return length(t . . a)
}

def operator ?. length(t : Goo) {
    return length(t . . a) + 1
}

[test]
def test_operators(t : T?) {
    t |> run("as, ?as, is") @(t : T?) {
        var f = Foo(a = 13)
        let b = f as Bar
        t |> equal(13., b.a)
        t |> success(b is Bar)
        t |> success(!(f is Bar))
        var pf = unsafe(addr(f))
        let pb = pf ?as Bar ?? Bar(a = 0.0)
        t |> equal(13., pb.a)
        var nf : Foo?
        let nb = nf ?as Bar ?? Bar(a = 1.0)
        t |> equal(1., nb.a)
    }
    t |> run("[], ?[], ., ?.") @(t : T?) {
        var T = Goo(a = "hello")
        let z = T["blah"]
        let w = T?["blah"] ?? "not_blah"
        let W = T?["hello"] ?? "this???"
        let EZ = T.GooBar
        let HARD_YES = T?.yes ?? "how??"
        let HARD_NO = T?.no ?? "hard no"
        t |> equal("hello-blah", z)
        t |> equal("not_blah", w)
        t |> equal("hello", W)
        t |> equal("hello.GooBar", EZ . . a)
        t |> equal("hello", HARD_YES)
        t |> equal("hard no", HARD_NO)
        t |> equal(5, T.length)
        t |> equal(6, T?.length)

        var bar : BarOp? <- new BarOp(a = 42)
        t |> equal(42, bar.Prop)
    }
}

struct BitArray {
    bits : uint64
    def operator [] (index : int) : bool {
        assert(index >= 0 && index < 64)
        let mask = (1ul << uint64(index))
        return (bits & mask) != 0ul
    }
    def operator [] =(index : int, value : bool) {
        assert(index >= 0 && index < 64)
        let mask = (1ul << uint64(index))
        if (value) {
            bits = bits | mask
        } else {
            bits = bits & ~mask
        }
    }
    def static clear(var self : BitArray) {
        self.bits = 0ul
    }
}

[test]
def test_index_operators(t : T?) {
    t |> run("[], []=") @(t : T?) {
        var a : BitArray
        t |> equal(a.bits, 0ul)
        a[1] = true
        t |> equal(a.bits, 2ul)
        t |> equal(a[1], true)
        a.clear()
        // all bits
        for (bit in 0..64) {
            a[bit] = true
        }
        for (bit in 0..64) {
            t |> equal(a[bit], true)
        }
        t |> equal(a.bits, 0xfffffffffffffffful)
        for (bit in 0..64) {
            a[bit] = false
        }
        for (bit in 0..64) {
            t |> equal(a[bit], false)
        }
        t |> equal(a.bits, 0ul)
        a.clear()
        // checkerboard
        for (bit in 0..32) {
            a[bit * 2] = true
        }
        for (bit in 0..64) {
            t |> equal(a[bit], bit % 2 == 0)
        }
        a.clear()
        for (bit in 0..32) {
            a[bit * 2 + 1] = true
        }
        for (bit in 0..64) {
            t |> equal(a[bit], bit % 2 == 1)
        }
        a.clear()
    }
    t |> run("compound operators") @(t : T?) {
        var a : BitArray
        a.clear()
        a[0] ||= true
        t |> equal(a[0], true)
        a[0] &&= false
        t |> equal(a[0], false)
        a[0] ^^= true
        t |> equal(a[0], true)
        a[0] ^^= true
        t |> equal(a[0], false)
    }
}

struct ArraySlots {
    slots : array<array<int>>
    def operator [] <-(index : int, var value : array<int>) {
        slots[index] <- value
    }
}

struct NamedRows {
    rows : table<string; array<int>>
}

def operator [] <-(var n : NamedRows; key : string; var value : array<int>) {
    n.rows[key] <- value
}

struct RefGrid {
    cells : array<array<int>>
}

def operator [] (var g : RefGrid; index : int) : array<int>& {
    unsafe {
        return g.cells[index]
    }
}

[test]
def test_index_move_operator(t : T?) {
    t |> run("[]<- free form") @(t : T?) {
        var n : NamedRows
        var v <- [1, 2, 3]
        n["a"] <- v
        t |> equal(0, length(v))
        t |> equal(3, length(n.rows["a"]))
    }
    t |> run("[]<- method form") @(t : T?) {
        var s : ArraySlots
        s.slots |> resize(2)
        var v <- [4, 5]
        s[1] <- v
        t |> equal(0, length(v))
        t |> equal(2, length(s.slots[1]))
    }
    t |> run("move through operator [] ref") @(t : T?) {
        var g : RefGrid
        g.cells |> resize(1)
        var v <- [1, 2]
        g[0] <- v
        t |> equal(0, length(v))
        t |> equal(2, length(g.cells[0]))
    }
}
