options gen2
require dastest/testing_boost public
require _assign_operator_mod

typedef distinct SoundHandle = uint

def operator =(var dst : SoundHandle&; src : int) {
    dst !== SoundHandle(uint(src))
}

def operator =(var dst : uint&; src : SoundHandle) {
    dst !== *src
}

def operator !=(h : SoundHandle; z : int) : bool => *h != uint(z)

struct Sfx {
    hit : uint
    current : SoundHandle = 5
}

var g_handle : SoundHandle = 7

struct Counted {
    value : int
    copies : int
}

def operator =(var dst : Counted&; src : Counted) {
    dst !== src
    dst.copies++
}

def operator :=(var dst : Counted&; src : Counted) {
    dst !:= src
    dst.copies += 100
}

struct Box {
    items : array<int>
    moves : int
}

def operator <-(var dst : Box&; var src : array<int>) {
    dst.items <- src
    dst.moves++
}

class Holder {
    h : uint
    def operator =(src : int) {
        h = uint(src) * 2u
    }
}

[test]
def test_distinct_assign_operator(t : T?) {
    t |> run("init at every site goes through operator =") @(t : T?) {
        let local : SoundHandle = 3
        t |> equal(*local, 3u)
        t |> equal(*g_handle, 7u)
        let sfx = Sfx()
        t |> equal(*sfx.current, 5u)
        let made = Sfx(hit = 1u, current = 9)
        t |> equal(*made.current, 9u)
    }
    t |> run("assignment and comparison") @(t : T?) {
        var current : SoundHandle = 3
        t |> success(current != 0)
        current = 0
        t |> success(!(current != 0))
        t |> equal(*current, 0u)
    }
    t |> run("either side may be the distinct type") @(t : T?) {
        var sfx = Sfx()
        sfx.hit = SoundHandle(11u)
        t |> equal(sfx.hit, 11u)
        let raw : uint = SoundHandle(12u)
        t |> equal(raw, 12u)
    }
}

[test]
def test_same_type_overload_and_raw_forms(t : T?) {
    t |> run("an overload on the exact type wins over the builtin copy") @(t : T?) {
        var src : Counted
        src.value = 1
        var dst : Counted
        dst = src
        t |> equal(dst.value, 1)
        t |> equal(dst.copies, 1)
        let init : Counted = src
        t |> equal(init.copies, 1)
    }
    t |> run("init from a make-struct temporary runs the operator as well") @(t : T?) {
        let made = Counted(value = 4, copies = 0)
        t |> equal(made.copies, 1)
    }
    t |> run("the raw copy is the builtin copy") @(t : T?) {
        var src : Counted
        src.value = 2
        var dst : Counted
        dst !== src // nolint:STYLE011 the raw copy has no declaration form; the statement is the subject
        t |> equal(dst.value, 2)
        t |> equal(dst.copies, 0)
    }
    t |> run("clone overload and its raw form") @(t : T?) {
        var src : Counted
        src.value = 3
        var viaOp : Counted
        viaOp := src
        t |> equal(viaOp.copies, 100)
        var raw : Counted
        raw !:= src // nolint:STYLE011 the raw clone has no declaration form; the statement is the subject
        t |> equal(raw.copies, 0)
    }
}

[test]
def test_move_operator(t : T?) {
    t |> run("<- dispatches to operator <-, at init and at assignment") @(t : T?) {
        var first <- [1, 2, 3]
        var box : Box <- first
        t |> equal(length(box.items), 3)
        t |> equal(box.moves, 1)
        t |> equal(length(first), 0)
        var second <- [4]
        box <- second
        t |> equal(length(box.items), 1)
        t |> equal(box.moves, 2)
    }
    t |> run("the raw move is the builtin move") @(t : T?) {
        var a : Box
        var b : Box
        b.moves = 5
        a !<- b // nolint:PERF030 a is fresh; the raw move has no declaration form to fold into
        t |> equal(a.moves, 5)
        t |> equal(b.moves, 0)
    }
}

struct Wrap {
    v : int
}

def operator =(var dst : Wrap&; src : auto(TT)) {
    dst.v = int(src) * 10
}

struct Sink {
    got : int
}

def operator =(var dst : Sink&; var src : array<int>) {
    dst.got = length(src)
    src |> push(0)
}

[test]
def test_init_reaches_generic_and_mutable_source_overloads(t : T?) {
    t |> run("a generic operator = serves init the way it serves assignment") @(t : T?) {
        var w : Wrap = 3.5f
        t |> equal(w.v, 30)
        w = 4
        t |> equal(w.v, 40)
        let made = WrapBox(w = 2.5f)
        t |> equal(made.w.v, 20)
    }
    t |> run("an operator = taking var src sees the mutable source at init") @(t : T?) {
        var arr <- [1, 2]
        var s : Sink = arr
        t |> equal(s.got, 2)
        t |> equal(length(arr), 3)
        s = arr
        t |> equal(s.got, 3)
        t |> equal(length(arr), 4)
    }
}

[inline]
def pass_through(src : Counted) : Counted {
    return src
}

def with_finally(src : Counted) : int {
    let held : Counted = src
    var total = 0
    {
        total += held.copies
    } finally {
        total += held.copies * 10
    }
    return total
}

[test]
def test_compiler_made_copies_stay_builtin(t : T?) {
    t |> run("an inlined return store does not run the operator a second time") @(t : T?) {
        var src : Counted
        src.value = 8
        let got : Counted = pass_through(src)
        t |> equal(got.value, 8)
        t |> equal(got.copies, 1)
    }
    t |> run("a let relocated for finally still runs the operator once") @(t : T?) {
        var src : Counted
        src.value = 9
        t |> equal(with_finally(src), 11)
    }
}

struct WrapBox {
    w : Wrap
}

struct Tracked {
    value : int
    log : int
}

def operator :=(var dst : Tracked&; src : Tracked) {
    dst !:= src
    dst.log += 100
}

def operator <-(var dst : Tracked&; var src : Tracked) {
    dst !<- src
    dst.log++
}

struct Node {
    v : int
}

struct Owner {
    p : Node?
}

def operator <-(var dst : Owner&; var src : Node?&) {
    dst.p = src
    src = null
}

struct Pt {
    a : int
    b : int
}

def operator <-(var dst : Sink&; src : Pt) {
    dst.got = src.a + src.b
}

def count_in_generator(src : Counted) : int {
    var total = -1
    for (n in generator<int>() <| $() { // nolint:STYLE002 a generator source inside a for takes only the piped block form
        var c : Counted = src
        yield c.copies
        return false
    }) {
        total = n
    }
    return total
}

def return_in_loop(src : Counted) : Counted {
    for (_i in range(2)) {
        {
            return src
        } finally {
            g_finally_hits++
        }
    }
    return src
}

var g_finally_hits = 0

[test]
def test_every_copy_the_compiler_makes_is_builtin(t : T?) {
    t |> run("a clone init runs operator := and not operator <- after it") @(t : T?) {
        var src : Tracked
        src.value = 5
        let viaOp : Tracked := src
        t |> equal(viaOp.log, 100)
        var glob : Tracked
        glob := src
        t |> equal(glob.log, 100)
    }
    t |> run("a generator local with a written type runs the operator once") @(t : T?) {
        var src : Counted
        src.value = 6
        t |> equal(count_in_generator(src), 1)
    }
    t |> run("a ternary with an inlined arm runs the operator once") @(t : T?) {
        var src : Counted
        src.value = 7
        let other = Counted()
        let pick = src.value > 3
        let got : Counted = pick ? pass_through(src) : other
        t |> equal(got.value, 7)
        t |> equal(got.copies, 1)
    }
    t |> run("a return inside a loop with finally stays a builtin copy") @(t : T?) {
        var src : Counted
        src.value = 8
        let got : Counted = return_in_loop(src)
        t |> equal(got.value, 8)
        t |> equal(got.copies, 1)
        t |> equal(g_finally_hits, 1)
    }
    t |> run("a lambda capture is a builtin copy") @(t : T?) {
        var src : Counted
        src.value = 9
        let peek = @() : int {
            return src.copies
        }
        t |> equal(invoke(peek), 0)
    }
}

[test]
def test_init_reaches_the_source_by_reference(t : T?) {
    t |> run("a pointer source moved at init is emptied like at assignment") @(t : T?) {
        var first = new Node(v = 1)
        var owner : Owner <- first
        t |> success(first == null)
        t |> equal(owner.p.v, 1)
        unsafe {
            delete owner.p
        }
        var second = new Node(v = 2)
        owner <- second
        t |> success(second == null)
        t |> equal(owner.p.v, 2)
        unsafe {
            delete owner.p
        }
    }
    t |> run("an operator <- taking a const source serves init") @(t : T?) {
        let pt = Pt(a = 1, b = 2)
        let s : Sink <- pt
        t |> equal(s.got, 3)
    }
}

[test]
def test_raw_forms_keep_builtin_clones(t : T?) {
    t |> run("a raw clone still reaches the builtin generic clone of an array") @(t : T?) {
        var src <- [1, 2, 3]
        var dst : array<int>
        dst !:= src
        t |> equal(length(dst), 3)
        t |> equal(length(src), 3)
    }
    t |> run("a raw move takes a table literal on the right") @(t : T?) {
        var tab : table<int; int>
        tab !<- { 1 => 2 } // nolint:PERF030,STYLE011 the raw move has no declaration form; the statement is the subject
        t |> equal(length(tab), 1)
    }
}

def operator <-(var dst : Box&; var src : Box) {
    dst !<- src // nolint:PERF030 the raw move IS this operator's primitive; the test's targets hold no container contents
    dst.moves += 10
}

[inline]
def pick(src : Counted; other : Counted; first : bool) : Counted {
    if (first) {
        return src
    }
    return other
}

def with_finally_early_out(src : Counted; bail : bool) : int {
    var total = 0
    let held : Counted = src
    {
        if (bail) {
            return -1
        }
        total += held.copies
    } finally {
        total += held.copies * 10
    }
    return total
}

[test]
def test_more_compiler_made_copies(t : T?) {
    t |> run("the raw move skips a same-type operator <- that the plain move takes") @(t : T?) {
        var a : Box
        var b : Box
        b.moves = 5
        a !<- b // nolint:PERF030 a is fresh; the raw move has no declaration form to fold into
        t |> equal(a.moves, 5)
        var c : Box
        c.moves = 6
        a <- c
        t |> equal(a.moves, 16)
    }
    t |> run("an inlined function with two returns stores its result raw") @(t : T?) {
        var src : Counted
        src.value = 10
        let other = Counted()
        let got : Counted = pick(src, other, true)
        t |> equal(got.value, 10)
        t |> equal(got.copies, 1)
    }
    t |> run("a let used in a finally after an early out still runs the operator once") @(t : T?) {
        var src : Counted
        src.value = 11
        t |> equal(with_finally_early_out(src, false), 11)
        t |> equal(with_finally_early_out(src, true), -1)
    }
    t |> run("a yielded value is a builtin copy") @(t : T?) {
        var src : Counted
        src.value = 12
        var seen = -1
        for (c in generator<Counted>() <| $() { // nolint:STYLE002 a generator source inside a for takes only the piped block form
            yield src
            return false
        }) {
            seen = c.copies
        }
        t |> equal(seen, 0)
    }
    t |> run("an operator declared in another module serves init and assignment here") @(t : T?) {
        var rm : Remote = 2
        t |> equal(rm.r, 6)
        rm = 3
        t |> equal(rm.r, 9)
    }
}

struct Wide {
    tag : int
}

def operator =(var dst : Wide&; src : auto(TT)) {
    dst.tag = 1
}

def operator =(var dst : Wide&; src : array<auto(TT)>) {
    dst.tag = 2 + length(src)
}

struct Keyed {
    value : int = 0
    copies : int = 0
}

def operator =(var dst : Keyed&; src : Keyed) {
    dst !== src
    dst.copies++
}

[test]
def test_init_resolution_matches_assignment(t : T?) {
    t |> run("two generic overloads resolve by specialization at init as at assignment") @(t : T?) {
        let arr <- [1, 2, 3]
        var w : Wide = arr
        t |> equal(w.tag, 5)
        w = 4
        t |> equal(w.tag, 1)
        let w2 : Wide = 7
        t |> equal(w2.tag, 1)
    }
    t |> run("an init from a table read of a type with field defaults still runs the operator") @(t : T?) {
        var tab : table<int; Keyed>
        tab[1].value = 9
        let got : Keyed = tab[1]
        t |> equal(got.value, 9)
        t |> equal(got.copies, 1)
        var again = Keyed()
        again = tab[1]
        t |> equal(again.copies, 1)
    }
}

[test]
def test_class_operator(t : T?) {
    t |> run("an operator = declared in a class takes self as the destination") @(t : T?) {
        var holder = new Holder()
        *holder = 21
        t |> equal(holder.h, 42u)
        unsafe {
            delete holder
        }
    }
}
