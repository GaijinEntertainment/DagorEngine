options gen2

require llvm_tune_scope_client_lib
require llvm_tune_scope_client_lib2

// App-face fixture for llvm_tune_scope.das (no [test] on purpose — the suite just compiles
// it): inherits BOTH libraries' scopes with zero declarations of its own; the runner drives
// the policy flavors via DAS_TUNE_POLICY and `-- --tune`. RESULT is the k-fingerprint of the
// stamped tier (jit: 2+3+k; interp reference: 5).
[export, tune_policy(missing = "fallback")]
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
