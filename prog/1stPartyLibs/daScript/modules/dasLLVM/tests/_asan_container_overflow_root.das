options gen2

//! reads element `k` through a raw pointer, where `k` is past the length but inside the capacity
[export, sideeffects, hint(noinline)]
def read_past_length(var a : array<int>; k : int) : int {
    var p = unsafe(addr(a[0]))
    return unsafe(p[k])
}

[export]
def main() {
    var a : array<int>   // nolint:STYLE012 - pushes grow the capacity past the length; a literal allocates exactly three
    a |> push(1)
    a |> push(2)
    a |> push(3)
    let k = length(get_command_line_arguments()) > 1000 ? 4 : 3
    print("READ_PAST {read_past_length(a, k)} CAPACITY {capacity(a)}\n")
}
