options gen2
options persistent_heap

// Repro: context mismatch in dastest benchmarks.
// Prints context name to prove benchmark runs in wrong context.

require dastest/testing_boost
require daslib/bool_array
require daslib/rtti

// Benchmark that prints context name and tries to iterate BoolArray
[benchmark]
def benchmark_context_check(b : B?) {
    unsafe {
        print("benchmark_context_check: context name = '{this_context().name}'\n")
    }
    var a : BoolArray
    for (i in 0 .. 10) {
        a.push(i % 2 == 0)
    }
    b |> run("for_each", 10) <| $() {
        var count = 0
        for (e in a) {
            if (e) {
                count ++
            }
        }
        feint("{count}\n")
    }
}
