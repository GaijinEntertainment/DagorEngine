options gen2

require llvm_tune_scope_client_lib
require llvm_tune_scope_client_lib2
require llvm/daslib/llvm_tune

// App-face fixture for llvm_tune_scope.das proving tune_suppress_mint: a library's [init]
// (the dev-run escape, e.g. DASLLAMA_ALLOW_UNTUNED=1) suppresses the auto-default MINT —
// untuned scopes keep fallback stamps instead of running the tuner.
[init]
def suppress_the_mint {
    tune_suppress_mint("TEST_SUPPRESS=1")
}

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
