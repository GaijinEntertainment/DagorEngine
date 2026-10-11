// an operator = marked [unsafe_operation] needs an unsafe block at an initialization the same way it does at an assignment
options gen2
expect 31013:1

struct Slot {
    v : int
}

[unsafe_operation]
def operator =(var dst : Slot&; src : Slot) {
    dst.v = src.v + 1
}

[export]
def main {
    var other : Slot
    other.v = 6
    let s : Slot = other
    print("{s.v}\n")
}
