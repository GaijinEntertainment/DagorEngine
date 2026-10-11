options gen2
// the dead function must survive into the AST the emitter walks: with symbol removal on it is
// gone before emission and the case proves nothing
options remove_unused_symbols = false

// only a DEAD function reaches fio_core - through its FILE? parameter and an extern call in its
// body - so the module stays compile-time-only and nothing registers

require daslib/fio

def dead_helper(f : FILE?) : bool {
    return f == null && has_env_variable("PATH")
}

[export]
def answer : int {
    return 42
}
