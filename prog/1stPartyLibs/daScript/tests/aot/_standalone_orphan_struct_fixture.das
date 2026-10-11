options gen2

// a standalone context emits its own module's structures even where nothing reaches them, so
// an unreferenced one still has to keep its field's module alive

require daslib/fio

struct Orphan {
    handle : FILE?
}

[export]
def answer() : int {
    return 42
}
