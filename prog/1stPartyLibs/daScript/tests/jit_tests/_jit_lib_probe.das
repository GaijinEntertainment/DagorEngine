options gen2


require daslib/export_c

struct Pair {
    lo : int
    hi : int
}

var g_seen = 0

[export_c]
def widen(p : Pair) : Pair {
    g_seen++
    return Pair(lo = p.lo - 1, hi = p.hi + 1)
}

[export_c(name = "tally")]
def count_seen() : int {
    return g_seen
}

[export_c]
def mapped(n : int) : int {
    let fx <- @@(x : int) => x * 3
    return invoke(fx, n)
}

def helper_public(x : int) : int {
    return x + 1
}
