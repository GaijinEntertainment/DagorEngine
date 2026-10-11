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

// ===== raw (original) operator family — `!` before an access/test operator bypasses the overload =====

def operator [] (_a : array<float>; _i : int) : float {
    return -999.0
}

def operator ?[] (_a : array<float>; _i : int) : float? {
    return null
}

def operator [] =(var _a : array<float>; _i : int; _v : float) {
    pass    // a lying setter: swallows the write
}

struct Qoo {
    v : int
}

def operator ??(_p : Qoo? const; _d : Qoo const) : Qoo {
    return Qoo(v = -777)
}

variant IntFloat {
    i : int
    f : float
}

def operator is i(_v : IntFloat) {
    return false
}

def operator as i(_v : IntFloat) {
    return -888
}

def operator ?as i(_v : IntFloat) : int? {
    return null
}

def operator is int(_g : Goo) {
    return true
}

[test]
def test_raw_operators(t : T?) {
    t |> run("!. and !?. reach the real field") @(t : T?) {
        var g = Goo(a = "hello")
        t |> equal("hello", g!.a)                    // g.GooBar promotes, g!.a does not
        t |> equal("hello.GooBar", g.GooBar!.a)
        t |> equal(g . . a, g!.a)                    // legacy spelling stays equivalent
        var pg = unsafe(addr(g))
        t |> equal("hello", pg!?.a ?? "dflt")
        var ng : Goo?
        t |> equal("dflt", ng!?.a ?? "dflt")
    }
    t |> run("![ and !?[ reach the real element") @(t : T?) {
        let xs <- [1.0, 2.0, 3.0]
        t |> equal(-999.0, xs[1])                    // the overload shadows builtin indexing
        t |> equal(2.0, xs![1])
        t |> equal(-1.0, xs?[1] ?? -1.0)             // the ?[] overload yields null
        t |> equal(2.0, xs!?[1] ?? -1.0)
        t |> equal(-1.0, xs!?[9] ?? -1.0)            // raw safe-index still bounds-checks
    }
    t |> run("![ bypasses the write path too") @(t : T?) {
        var ws <- [1.0, 2.0, 3.0]
        ws[1] = 55.0                                 // the lying []= swallows the write
        t |> equal(2.0, ws![1])
        ws![1] = 55.0                                // raw write reaches the real element
        t |> equal(55.0, ws![1])
        ws![1] += 1.0                                // nolint:PERF013 - raw compound write is the tested construct
        t |> equal(56.0, ws![1])
    }
    t |> run("!?? reaches builtin pointer coalescing") @(t : T?) {
        var q = Qoo(v = 13)
        var pq = unsafe(addr(q))
        t |> equal(-777, (pq ?? Qoo(v = 1)).v)       // the ?? overload wins
        t |> equal(13, (pq !?? Qoo(v = 1)).v)
        var nq : Qoo?
        t |> equal(1, (nq !?? Qoo(v = 1)).v)
    }
    t |> run("!is, !as, !?as reach the real variant") @(t : T?) {
        let vif = IntFloat(i = 42)
        t |> success(!(vif is i))                    // the lying `is` operator wins
        t |> success(vif !is i)
        t |> equal(-888, vif as i)                   // the lying `as` operator wins
        t |> equal(42, vif !as i)
        t |> equal(-1, vif ?as i ?? -1)              // the lying `?as` operator wins
        t |> equal(42, unsafe(vif !?as i) ?? -1)
    }
    t |> run("!is type form reaches the static check") @(t : T?) {
        let g = Goo(a = "x")
        t |> success(g is int)                       // the lying `is int` operator wins
        t |> success(!(g !is int))
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
