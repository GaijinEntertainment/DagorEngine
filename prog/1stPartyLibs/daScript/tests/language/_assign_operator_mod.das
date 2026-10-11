options gen2

module _assign_operator_mod shared public

struct public Remote {
    r : int
}

def public operator =(var dst : Remote&; src : int) {
    dst.r = src * 3
}
