options gen2

expect 30254

[export]
def main {
    var tab : table<half4; int>     // 30254: a 16-bit lattice vector is not a hashable key, in a declaration as in a literal
}
