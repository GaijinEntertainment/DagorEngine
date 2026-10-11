options gen2
options _dasllama_internal = true

require dasllama/dasllama_tune

// the stale-sidecar client: DAS_TUNE_MANIFEST points it at a manifest whose entry for
// stale_probe_dot names a perm no grid offers - the compile must survive on the fallback
def template stale_probe_dot_template(a, b : array<float>) : float {
    var s = 0f
    for [tune = 1] (i in range(length(a))) {   // nolint:PERF029 - the tuned kernel: perm hints stamp this index loop
        s += a[i] * b[i]
    }
    return s
}

[tuned]
def stale_probe_dot(a, b : array<float>) : float {}

[export]
def main {
    let a = [for (i in range(8)); float(i)]
    print("STALE_CLIENT_OK {stale_probe_dot(a, a)}\n")
}
