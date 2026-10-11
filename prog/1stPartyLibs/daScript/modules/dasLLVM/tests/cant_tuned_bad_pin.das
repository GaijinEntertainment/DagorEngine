// an explicit `perm=` pin naming a perm outside the grid is an authoring error and keeps the
// hard compile refusal - the stale-SIDECAR fallback in dasllama_tune's [tuned] must not eat it
// (the sidecar arm is the positive twin: test_tuned's stale-sidecar cell)
options gen2
expect 20800:1
options _dasllama_internal = true

require dasllama/dasllama_tune

def template bad_pin_dot_template(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {   // nolint:PERF029 - the tuned kernel: perm hints stamp this index loop
        s += a[i] * b[i]
    }
    return s
}

[tuned(perm = "no_such_perm")]
def bad_pin_dot(a, b : array<float>) : float {}

[export]
def main {
    print("never compiles\n")
}
