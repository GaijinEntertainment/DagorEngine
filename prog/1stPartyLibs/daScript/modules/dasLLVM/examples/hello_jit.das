options gen2
require daslib/just_in_time // the JIT's entry, what -jit requires: every [jit] function is jitted

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

