// Squirrel module under test: dagor.perf (registered by sqratDagorDebug.cpp)

from "dagor.perf" import get_avg_cpu_only_cycle_time_usec, reset_summed_cpu_only_cycle_time //disable: -see-other


// ============================================================
// Section 1: get_avg_cpu_only_cycle_time_usec returns float
// ============================================================
println("--- Section 1: get_avg_cpu_only_cycle_time_usec ---")

let v = get_avg_cpu_only_cycle_time_usec()
assert(typeof v == "float", $"get_avg_cpu_only_cycle_time_usec type: {typeof v}")
assert(v >= 0.0, $"non-negative: {v}")

println("Section 1 PASSED")


// ============================================================
// Section 2: reset_summed_cpu_only_cycle_time is total
// ============================================================
println("--- Section 2: reset_summed_cpu_only_cycle_time is total ---")

reset_summed_cpu_only_cycle_time()
let v2 = get_avg_cpu_only_cycle_time_usec()
assert(v2 >= 0.0, $"non-negative after reset: {v2}")

println("Section 2 PASSED")


// ============================================================
// Section 3: incorrect input
// ============================================================
// dagor.perf exposes only zero-argument total accessors. The smallest
// deterministic incorrect-input test we can do is to pass an extra argument
// to a zero-arg native closure.
println("--- Section 3: incorrect input ---")

local threw = false
try {
  get_avg_cpu_only_cycle_time_usec("extra") //disable:-param-count
} catch (e) {
  threw = true
}
assert(threw, "extra arg to zero-arg native closure must throw")

println("Section 3 PASSED")


println("ALL TESTS PASSED")
