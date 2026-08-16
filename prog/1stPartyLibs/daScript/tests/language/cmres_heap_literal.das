options gen2
options indenting = 4

// A local that aliases the return CMRES (`var r` + `return <- r` copy elision) read as a
// call argument inside a HEAP-mode array literal: SimNode_MakeArrayHeap repoints abiCMRES
// at the heap block while constructing elements, so without the VarCMRes fence those reads
// resolved into the zeroed heap buffer instead of r (all args read 0). Auto-inline
// manufactures this shape when a by-ref callee splices into a CMRES-elided local.

require dastest/testing_boost public

struct Rec {
    s : uint64
    bind : uint
    items : array<int>
}

struct MiniRes {
    set0 : uint64
    set1 : uint64
}

def private mk(h : uint64; b : uint) : Rec {
    var w = Rec(s = h, bind = b)
    w.items |> push(int(b))
    return <- w
}

def private consume(r : MiniRes) : uint64 {
    // any gen2 [..] literal lowers to to_array_move(make-array) and is flagged heap-mode
    // unconditionally (element count is irrelevant; probe-verified with a 3-element literal)
    var writes <- [
        mk(r.set0, 0u), mk(r.set0, 1u), mk(r.set0, 2u), mk(r.set0, 3u), mk(r.set0, 4u),
        mk(r.set0, 5u), mk(r.set0, 6u), mk(r.set0, 7u), mk(r.set0, 8u),
        mk(r.set1, 9u), mk(r.set1, 10u), mk(r.set1, 11u), mk(r.set1, 12u), mk(r.set1, 13u),
        mk(r.set1, 14u), mk(r.set1, 15u), mk(r.set1, 16u), mk(r.set1, 17u), mk(r.set1, 18u)]
    var sum = 0ul
    for (w in writes) {
        sum += w.s
    }
    return sum
}

def build_direct : MiniRes {
    // the literal lives in THIS function, next to the elided return local
    var r = MiniRes(set0 = 0x1111ul, set1 = 0x2222ul)
    var writes <- [
        mk(r.set0, 0u), mk(r.set0, 1u), mk(r.set0, 2u), mk(r.set0, 3u), mk(r.set0, 4u),
        mk(r.set0, 5u), mk(r.set0, 6u), mk(r.set0, 7u), mk(r.set0, 8u),
        mk(r.set1, 9u), mk(r.set1, 10u), mk(r.set1, 11u), mk(r.set1, 12u), mk(r.set1, 13u),
        mk(r.set1, 14u), mk(r.set1, 15u), mk(r.set1, 16u), mk(r.set1, 17u), mk(r.set1, 18u)]
    var sum = 0ul
    for (w in writes) {
        sum += w.s
    }
    r.set0 = sum        // 9*0x1111 + 10*0x2222 = 0x9999 + 0x15554 = 0x1EEED
    return <- r
}

def build_via_callee : MiniRes {
    // the literal arrives via a by-ref callee — the shape auto-inline splices into
    // the CMRES-elided local (works with the heuristic tier on OR off)
    var r = MiniRes(set0 = 0x1111ul, set1 = 0x2222ul)
    r.set0 = consume(r)
    return <- r
}

[test]
def test_cmres_heap_literal_direct(t : T?) {
    let r <- build_direct()
    t |> equal(r.set0, 0x1EEEDul)
    t |> equal(r.set1, 0x2222ul)
}

[test]
def test_cmres_heap_literal_via_callee(t : T?) {
    let r <- build_via_callee()
    t |> equal(r.set0, 0x1EEEDul)
    t |> equal(r.set1, 0x2222ul)
}
