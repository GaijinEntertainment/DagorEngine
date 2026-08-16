options gen2

require llvm_tune_scope_client_lib
require llvm_tune_scope_client_lib2

// App-face fixture for llvm_tune_scope.das with NO [tune_policy] — the "untuned does not
// start" default: an incomplete/stale sidecar must tune-on-start + re-exec with zero app
// declarations (DAS_TUNE_POLICY=fallback is the kill switch the runner also probes).
[export]
def main {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    print("RESULT {scope_add(n, 3)}\n")
    print("RESULT2 {scope_mul(n, 3)}\n")
    var st <- tune_status()
    for (s in st) {
        print("STATUS {s.fname}|{s.scope}|{s.suffix}|{s.source}\n")
    }
    delete st
}
