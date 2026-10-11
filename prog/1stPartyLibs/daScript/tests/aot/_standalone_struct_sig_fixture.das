options gen2

struct Pair {
    a : int
    b : int
}

enum Mode {
    off
    on
}

[export]
def make_pair(x, y : int) : Pair {
    return Pair(a = x, b = y)
}

[export]
def pair_sum(p : Pair) : int {
    return p.a + p.b
}

[export]
def flip(m : Mode) : Mode {
    return m == Mode.on ? Mode.off : Mode.on
}
