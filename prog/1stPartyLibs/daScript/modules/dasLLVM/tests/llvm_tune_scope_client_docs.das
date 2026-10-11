options gen2

require llvm_tune_scope_client_lib
require llvm_tune_scope_client_lib2 // nolint:STYLE029,STYLE030,LINT019 — intentionally loads the second tune scope; nothing of it is referenced on a host without dasLLVM

// A documentation root compiles the source API but neither stamps winners nor
// runs tune policy, even when the environment requests the strict error flavor.
[export]
def main {
    let n = 2
    print("RESULT {scope_add(n, 3)}\n")
}
