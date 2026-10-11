// the fallback arm of the stale-sidecar rescue keeps its own floor: a `fallback=` chain whose
// picked perm is outside the grid still refuses the compile ("tuned: fallback perm ... not in
// grid") - the sidecar rescue must never loop through a broken fallback (twin of
// cant_tuned_bad_pin.das; the sidecar arm is test_tuned's stale-sidecar cell)
options gen2
expect 20800:1
options _dasllama_internal = true

require dasllama/dasllama_tune

def template bad_fallback_dot_template(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {   // nolint:PERF029 - the tuned kernel: perm hints stamp this index loop
        s += a[i] * b[i]
    }
    return s
}

[tuned(fallback = "no_such_fallback")]
def bad_fallback_dot(a, b : array<float>) : float {}

[export]
def main {
    print("never compiles\n")
}
