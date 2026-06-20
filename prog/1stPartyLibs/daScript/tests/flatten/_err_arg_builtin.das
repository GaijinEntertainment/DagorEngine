options gen2
options indenting = 4

require daslib/flatten
require math

def identityf(a : float) : float {
    return a
}

// A non-whitelisted builtin (`tanh`) buried in the ARGUMENT of a user call, with a
// RUNTIME argument so it cannot const-fold. The argument must be lifted/whitelist-
// checked, not spliced verbatim — else the builtin slips past the gate (the cycle-2
// scan only catches surviving user calls, not non-whitelisted builtins).
[flatten]
def witharg(x : float) : float {
    return identityf(tanh(x))
}

[export]
def main {
    print("{witharg(0.5)}\n")
}
