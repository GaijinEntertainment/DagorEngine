options gen2
options indenting = 4

require daslib/validate_code

//! Companion to the fan-out fixture: the cycle here closes through `shared_tail`, which two
//! different branches reach. A memo that swallowed an already-visited function on the current
//! path would let the cycle through, so this fixture must still fail to compile.

[never_inline]
def shared_tail() : int {
    return branch_top()
}

[never_inline]
def branch_left() : int {
    return shared_tail()
}

[never_inline]
def branch_right() : int {
    return shared_tail()
}

[never_inline]
def branch_top() : int {
    return branch_left() + branch_right()
}

[verify_completion]
def cycle_root() : int {
    return branch_top()
}

[export]
def main {
    feint("{cycle_root()}")
}
