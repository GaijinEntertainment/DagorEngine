// Regression coverage for the heap_total_allocated / string_heap_total_allocated bindings:
// total reserved capacity must stay >= live bytes, be non-zero, and never shrink across an
// allocation. Catches a mis-wired binding / wrong return type / ABI drift. AOT-eligible (no
// options no_aot) so the AOT codegen path for these builtins is exercised too.
options gen2

require dastest/testing_boost public

// global → the allocation escapes to the heap (escape analysis won't stack-allocate it)
var g_sink : array<int>

[test]
def test_heap_total_allocated(t : T?) {
    t |> run("total reserved >= live bytes (both heaps)") @(t : T?) {
        t |> success(heap_total_allocated() >= heap_bytes_allocated(),
            "value heap: total reserved must be >= live bytes")
        t |> success(string_heap_total_allocated() >= string_heap_bytes_allocated(),
            "string heap: total reserved must be >= live bytes")
    }
    t |> run("totals are sane and non-decreasing after a large allocation") @(t : T?) {
        let t0 = heap_total_allocated()
        let b0 = heap_bytes_allocated()
        g_sink |> resize(1048576)          // ~4 MB on the value heap
        for (i in range(length(g_sink))) {
            g_sink[i] = i
        }
        let t1 = heap_total_allocated()
        let b1 = heap_bytes_allocated()
        t |> success(b1 > b0, "live bytes must grow after the allocation")
        t |> success(t1 >= b1, "total reserved must still be >= live bytes")
        t |> success(t1 >= t0, "total reserved must not shrink across an allocation")
        t |> success(t1 > 0ul, "total reserved must be non-zero")
        g_sink |> resize(0)
    }
}
