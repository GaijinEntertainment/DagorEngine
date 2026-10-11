options gen2

require daslib/tune
require llvm_tune_scope_client_verlib
require llvm_tune_scope_client_verlib2

// App-face fixture for the version-pin arms of llvm_tune_scope.das (no [test] on purpose):
// RESULT is the k-fingerprint of the stamped tier (jit: 2+3+k; interp reference: 5).
[export, tune_policy(missing = "fallback")]
def main {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    print("RESULT {ver_add(n, 3)}\n")
    print("RESULT2 {ver2_add(n, 3)}\n")
}
