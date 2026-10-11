options gen2

[export, never_inline]
def returned_a() : string {
    return "x\x00a"
}

[export, never_inline]
def returned_b() : string {
    return "x\x00b"
}

[export, never_inline]
def compared_a(s : string) : bool {
    return s == "x\x00a"
}

[export, never_inline]
def compared_b(s : string) : bool {
    return s == "x\x00b"
}

[export, never_inline]
def plain_a() : string {
    return "xa"
}

[export, never_inline]
def plain_b() : string {
    return "xa"
}
