options gen2
require dastest/testing_boost
require daslib/rtti

let BUDGET_NS_LESS_TIMER_SLACK = 900_000_000l

var g_stats = BenchmarkRunStats()
var g_starts = 0
var g_sink = 0

def make_bench(name : string) : B? {
    var b = new B(name = name)
    b.on_start <- @(sub_name : string) {
        g_starts++
    }
    b.on_finished <- @(stats : BenchmarkRunStats) {
        g_stats = stats
    }
    b.on_fail <- @(now : bool) {}
    b.on_log <- @(msg : string; at : LineInfo) {}
    return b
}

[test]
def test_reported_batch_meets_the_budget(t : T?) {
    t |> run("a fast op grows to a batch spanning the budget less timer slack") @@(t : T?) {
        var b = make_bench("budget")
        g_starts = 0
        b |> run("fast") {
            g_sink++
        }
        t |> equal(g_starts, 1, "the name is announced once")
        t |> success(g_stats.n > 1, "grew past the calibration call: n = {g_stats.n}")
        t |> success(g_stats.n < 1000 || g_stats.n % 10 == 0, "n = {g_stats.n} is a whole number of unrolled groups")
        t |> success(g_stats.time_ns >= BUDGET_NS_LESS_TIMER_SLACK, "reported batch spans {g_stats.time_ns} ns, needs >= {BUDGET_NS_LESS_TIMER_SLACK}")
        unsafe {
            delete b
        }
    }
    t |> run("a slow op is reported from the batch that met the budget") @@(t : T?) {
        var b = make_bench("budget")
        b |> run("slow") {
            var acc = 0
            for (i in range(20_000_000)) {
                acc += i & 7
            }
            g_sink += acc
        }
        t |> success(g_stats.time_ns >= BUDGET_NS_LESS_TIMER_SLACK, "reported batch spans {g_stats.time_ns} ns over n = {g_stats.n}, needs >= {BUDGET_NS_LESS_TIMER_SLACK}")
        unsafe {
            delete b
        }
    }
    feint("{g_sink}")
}
