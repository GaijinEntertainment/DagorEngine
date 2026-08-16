options gen2
require jit
require llvm/daslib/llvm_macro // This require will jit all functions

[jit]
def add(a, b : int) {
    return a + b
}

[export]
def main {
    verify(is_jit_function(@@add))
    let c = add(11, 12)
    print("result = {c}\n")
}

