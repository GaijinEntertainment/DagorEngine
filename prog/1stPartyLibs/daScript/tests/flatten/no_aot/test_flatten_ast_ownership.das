options gen2
options indenting = 4
options persistent_heap

require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost

//! Pins the ownership rule the flatten optimizer's `unsafe { delete <container> }` sites rest on.
//! `ExpressionPtr` is `Expression?` — a HANDLED C++ type, so its instances never live on the das
//! heap and the container finalizer frees only the buffer. That is what makes it safe to delete an
//! `array<ExpressionPtr>` still holding borrowed tree nodes (the const-fold early return of
//! `make_float_ctor` leaves its lanes un-consumed) and to keep a borrowed node in a plain struct
//! field without `@do_not_delete`. If a release ever puts AST nodes on the das heap, those deletes
//! become live-tree frees and this test is the tripwire — it fails here, not in a shader.
//!
//! The das-heap arm is the anti-vacuity control: the same measurement over `array<S?>` must show
//! the pointees actually reclaimed, or the AST arm proves nothing.
//!
//! Lives in tests/flatten/no_aot/: `options persistent_heap` (exact free accounting) is a
//! whole-program policy the AOT batch does not carry.

struct private HeapNode {
    v : int
}

struct private BorrowHolder {
    lane : int
    e : ExpressionPtr
}

def private borrowed_lane_survives_delete(t : T?) {
    var owner = new ExprCall(name := "float2")
    var a0 : ExpressionPtr = new ExprConstFloat(value = 1.0)
    var a1 : ExpressionPtr = new ExprConstFloat(value = 2.0)
    emplace(owner.arguments, a0)
    emplace(owner.arguments, a1)
    let spelling = "{describe(owner)}"
    var borrowed <- [owner.arguments[0], owner.arguments[1]]
    unsafe {
        delete borrowed
    }
    t |> equal("{describe(owner)}", spelling, "an array delete must not reach the borrowed lanes")
    var held <- [BorrowHolder(lane = 0, e = owner.arguments[0]), BorrowHolder(lane = 1, e = owner.arguments[1])]
    unsafe {
        delete held
    }
    t |> equal("{describe(owner)}", spelling, "a struct-field delete must not reach the borrowed lanes either")
}

[test]
def test_flatten_ast_ownership(t : T?) {
    t |> run("deleting an array of das-heap pointers reclaims the pointees") @(t : T?) {
        let base = heap_bytes_allocated()
        var arr <- [for (i in range(4)); new HeapNode(v = i)]
        t |> success(heap_bytes_allocated() > base, "four heap nodes plus a buffer must show on the heap")
        unsafe {
            delete arr
        }
        t |> equal(heap_bytes_allocated(), base, "the pointees must come back, not just the buffer")
    }
    t |> run("deleting a container of AST pointers frees the buffer only") @(t : T?) {
        ast_gc_guard() {
            borrowed_lane_survives_delete(t)
        }
    }
}
