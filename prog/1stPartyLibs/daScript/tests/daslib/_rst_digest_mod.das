options gen2
options no_aot

module _rst_digest_mod

let public DIGEST_LIMIT = 3

enum public DigestKind {
    first
    second
}

typedef public DigestFlags = bitfield<one; two>

struct public DigestPoint {
    x : int
    y : int
}

class public DigestBox {
    def show : int {
        return 1
    }
}

def public digest_pair(a : int) : int {
    return a
}

def public digest_pair(a, b : int) : int {
    return a + b
}

def public digest_lonely(a : int) : int {
    return a
}
