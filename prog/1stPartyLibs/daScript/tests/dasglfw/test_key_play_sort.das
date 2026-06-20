options gen2
options no_aot

require dastest/testing_boost public

// Self-contained smoke test for the sort technique used by
// `sort_key_play_events` in modules/dasGlfw/dasglfw/glfw_live.das. We can't
// require live/glfw_live directly here without dragging in the full
// GLFW + OpenGL chain (which the no_aot harness can't link against),
// so we mirror the wire-event shape + comparator and verify the same
// sort() with a `<` lambda lands t_ms-ascending.
//
// `sort()` is qsort, not stable, so this test asserts only the
// ordering invariant key_play depends on (non-decreasing t_ms),
// not any tie-break ordering.

struct TestKeyEventWire {
    t_ms : int    = 0
    kind : string = "press"
    tag  : string = ""
}

def sort_by_t_ms(var events : array<TestKeyEventWire>) {
    events |> sort() $(a, b : TestKeyEventWire) {
        return a.t_ms < b.t_ms
    }
}

[test]
def test_sort_orders_by_tms(t : T?) {
    var events <- [
        TestKeyEventWire(t_ms = 300, kind = "release", tag = "c"),
        TestKeyEventWire(t_ms = 100, kind = "press",   tag = "a"),
        TestKeyEventWire(t_ms = 200, kind = "char",    tag = "b")
    ]
    sort_by_t_ms(events)
    t |> equal(events[0].t_ms, 100)
    t |> equal(events[1].t_ms, 200)
    t |> equal(events[2].t_ms, 300)
}

[test]
def test_sort_empty(t : T?) {
    var events : array<TestKeyEventWire>
    sort_by_t_ms(events)
    t |> equal(length(events), 0)
}

[test]
def test_sort_already_sorted(t : T?) {
    var events <- [
        TestKeyEventWire(t_ms = 100),
        TestKeyEventWire(t_ms = 200),
        TestKeyEventWire(t_ms = 300)
    ]
    sort_by_t_ms(events)
    t |> equal(events[0].t_ms, 100)
    t |> equal(events[1].t_ms, 200)
    t |> equal(events[2].t_ms, 300)
}
