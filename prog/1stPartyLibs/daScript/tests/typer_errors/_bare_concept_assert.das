options gen2
expect 31400

// A concept_assert with no message of its own, so the compiler has to supply the default one.
// It sits in a generic, which is the only place concept_assert is meaningful.

def private takes(x : auto(TT)) {
    concept_assert(false)
}

[export]
def main {
    takes(1)
}
