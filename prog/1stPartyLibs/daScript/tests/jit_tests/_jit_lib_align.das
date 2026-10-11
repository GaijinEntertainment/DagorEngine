options gen2


require daslib/export_c

struct Wide {
    a : int8
    b : int64
    c : float
}

[export_c]
def make(n : int) : Wide {
    return Wide(a = int8(n), b = int64(n) * 1000000000l, c = float(n) + 0.5)
}
