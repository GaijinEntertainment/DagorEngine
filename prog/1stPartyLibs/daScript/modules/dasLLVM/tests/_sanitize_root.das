options gen2

//! reads `n` ints into a four-int stack array - past its end for any n above 3; kept out of line, where
//! the optimizer cannot fold the out-of-bounds read of a known-size array before asan instruments it
[export, sideeffects, hint(noinline)]
def read_past(n : int) : int {
    var a : int[4]
    a[0] = 1
    var p = unsafe(addr(a[0]))
    return unsafe(p[n])
}

[export]
def main() {
    //! one past the end, inside the redzone asan puts after the array; the command line keeps the optimizer from folding it
    let n = length(get_command_line_arguments()) > 1000 ? 5 : 4
    print("READ_PAST {read_past(n)}\n")
}
