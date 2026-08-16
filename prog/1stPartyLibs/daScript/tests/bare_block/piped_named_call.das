options gen2

// a named-argument call can take a piped block. it lands on the block parameter,
// padding across defaults just like a regular piped call (so the block param may be
// last, behind named/defaulted arguments).
require dastest/testing_boost public

struct Holder {
    v : int
}

def setv(self : Holder; _caption : string; blk : block<() : int>; x : int = 0) : int {
    return self.v + x + invoke(blk)
}

def freefn(tag : int; blk : block<() : int>; scale : int = 1) : int {
    return tag + scale * invoke(blk)
}

// block parameter is LAST, behind a defaulted 'opt' -> needs padding
def sec(tag : int; opt : int = 0; body : block<() : int>) : int {
    return tag + opt + invoke(body)
}

// two defaults between the positional and the block param
def two(tag : int; a : int = 0; b : int = 0; body : block<() : int>) : int {
    return tag + a + b + invoke(body)
}

// a piped named call INSIDE a generic body: the body is cloned (clone_function) on
// instantiation, before it demotes - the clone must carry nonNamedArguments (the positional
// and the piped block, which lands last), else they are lost (ExprNamedCall::clone regression)
def in_generic(g) : int {
    return sec(g, opt = 3) {
        return 1
    }
}

// overload set - least padding wins (the 3-arg one, block closer)
def ov(a : int; x : int = 0; body : block<() : int>) : int { return 100 + a + x + invoke(body) }
def ov(a : int; x : int = 0; y : int = 0; body : block<() : int>) : int { return 200 + a + x + y + invoke(body) }

struct Panel { depth : int }
// free function called via dot (w.draw(...) == draw(w, ...)); block param last -> pads
def draw(self : Panel; _caption : string; indent : int = 0; body : block<() : int>) : int {
    return self.depth + indent + invoke(body)
}
// generic first argument, block param last -> pads
def gdraw(tag; opt : int = 0; body : block<() : int>) : int { return opt + invoke(body) }

// genuine class method (def inside class {}): a struct field, unreachable by bare name, so it
// resolves through the member-call path. block param last, behind a defaulted 'indent' -> pads.
class Frame {
    depth : int
    def region(_caption : string; indent : int = 0; body : block<() : int>) : int {
        return depth + indent + invoke(body)
    }
    // two defaults between the middle positional and the block param
    def region2(_caption : string; a : int = 0; b : int = 0; body : block<() : int>) : int {
        return depth + a + b + invoke(body)
    }
    // block param right after self+caption (no default between) -> no padding
    def plain(_caption : string; body : block<() : int>) : int {
        return depth + invoke(body)
    }
    // implicit-self member call from within another method, block param pads across 'indent'
    def wrap() : int {
        return region("hi", indent = 4) {
            return 3
        }
    }
}

// struct (not class) method with a block param last, behind a defaulted arg -> pads
struct Box {
    v : int
    def cell(_caption : string; pad : int = 0; body : block<() : int>) : int {
        return v + pad + invoke(body)
    }
}

[test]
def test_method_named_piped_block(t : T?) {
    let h = Holder(v = 100)
    let r = h.setv("go", x = 5) {
        return 20
    }
    t |> equal(r, 125)
}

[test]
def test_freefn_named_piped_block(t : T?) {
    let r = freefn(3, scale = 10) {
        return 4
    }
    t |> equal(r, 43)
}

[test]
def test_named_piped_padding(t : T?) {
    // block param last, named 'opt' in between -> block pads across opt
    let r1 = sec(5, opt = 3) {
        return 1
    }
    t |> equal(r1, 9)
    // still works with no named arg (block right after positional, no padding)
    let r2 = sec(5) {
        return 1
    }
    t |> equal(r2, 6)
    // two defaults, both named
    let r3 = two(1, a = 2, b = 3) {
        return 4
    }
    t |> equal(r3, 10)
    // two defaults, only the second named -> the first is padded
    let r4 = two(1, b = 3) {
        return 4
    }
    t |> equal(r4, 8)
}

[test]
def test_named_piped_overload(t : T?) {
    // least padding wins: the 3-arg ov (block at slot 2) beats the 4-arg one
    let r = ov(1, x = 2) {
        return 3
    }
    t |> equal(r, 106)
}

[test]
def test_dot_and_generic_padding(t : T?) {
    // dot-syntax free function, block param last, named 'indent' -> block pads across indent
    let p = Panel(depth = 100)
    let r1 = p.draw("hdr", indent = 5) {
        return 1
    }
    t |> equal(r1, 106)
    // generic first arg, block param last -> pads
    let r2 = gdraw(7, opt = 3) {
        return 2
    }
    t |> equal(r2, 5)
}

[test]
def test_class_method_padding(t : T?) {
    var f = new Frame(depth = 100)
    // middle positional "hdr" + named indent, block param last -> block pads across indent
    let r1 = f.region("hdr", indent = 5) {
        return 1
    }
    t |> equal(r1, 106)
    // block right after self+caption (no default between) -> no padding needed
    let r2 = f.plain("hdr") {
        return 9
    }
    t |> equal(r2, 109)
    // two defaults, only the second named -> the first is padded across too
    let r3 = f.region2("hdr", b = 3) {
        return 2
    }
    t |> equal(r3, 105)
    // implicit-self member call, block pads across a default
    t |> equal(f.wrap(), 107)
    unsafe { delete f }
}

[test]
def test_struct_method_padding(t : T?) {
    var b = new Box(v = 10)
    let r = b.cell("hdr", pad = 5) {
        return 1
    }
    t |> equal(r, 16)
    unsafe { delete b }
}

[test]
def test_piped_named_in_generic(t : T?) {
    // the piped named call lives in a generic body; the positional and block must survive the clone
    t |> equal(in_generic(5), 9)
}

[test]
def test_arrow_named_piped_block(t : T?) {
    // '->' arrow form parses to the same ExprNamedCall as '.', so the piped-block padding
    // applies identically. free function via dot-target struct:
    let h = Holder(v = 100)
    let r = h->setv("go", x = 5) {
        return 20
    }
    t |> equal(r, 125)
    // genuine class method via '->', block param last behind a defaulted 'indent' -> pads
    var f = new Frame(depth = 100)
    let r1 = f->region("hdr", indent = 5) {
        return 1
    }
    t |> equal(r1, 106)
    unsafe { delete f }
    // struct method via '->', block param last behind a defaulted 'pad' -> pads
    var b = new Box(v = 10)
    let r2 = b->cell("hdr", pad = 5) {
        return 1
    }
    t |> equal(r2, 16)
    unsafe { delete b }
}
