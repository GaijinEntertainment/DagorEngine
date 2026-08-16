// each_ref compile-time rejection: the lambda must take its argument as a reference to a
// pointer (var arg : T? &). The by-value shape can never write the iteration slot — it used
// to compile and crash at runtime; now the ==& trap overload rejects it with a clear message.
options gen2
// coverage instrumentation re-infers after its pass and would wrap the expected 31400 in an
// extra 31207 (macro failed to infer) — instrumenting a deliberately-failing compile is meaningless
options no_coverage = true

expect 31400:1

[export]
def main() {
    var idx = 0
    var lam <- @(var a : int?) : bool {
        if (idx >= 3) {
            return false
        }
        idx++
        return true
    }
    var n = 0
    for (x in each_ref(lam)) {
        n++
    }
}
