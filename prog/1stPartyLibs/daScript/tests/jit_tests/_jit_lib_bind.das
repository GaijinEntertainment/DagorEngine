options gen2


require daslib/export_c

struct Pair {
    lo : int
    hi : int
}

var g_calls = 0

[export_c]
def bump() : int {
    g_calls++
    return g_calls
}

[export_c]
def widen(p : Pair) : Pair {
    return Pair(lo = p.lo - 1, hi = p.hi + 1)
}

[export_c]
def greet(who : string) : string {
    return "hola {who}"
}

[export_c(name = "double_or_raise")]
def boom(n : int) : int {
    if (n < 0) {
        panic("negative: {n}")
    }
    return n * 2
}
