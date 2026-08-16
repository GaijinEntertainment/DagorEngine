options gen2

// child for llvm_compile_only.das: under `-jit -- --jit-compile-only` the module is built and
// verified but nothing is installed — main runs interpreted and must still compute correctly

def compute(a, b : int) : int => a * 100 + b

[export]
def main {
    print("COMPILE_ONLY_RESULT {compute(4, 2)}\n")
}
