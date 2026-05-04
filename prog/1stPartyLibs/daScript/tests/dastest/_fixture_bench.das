options gen2
require dastest/testing_boost

[benchmark]
def bench_simple(b : B?) {
    b |> run("add") {
        var x = 0
        x += 1
    }
}
