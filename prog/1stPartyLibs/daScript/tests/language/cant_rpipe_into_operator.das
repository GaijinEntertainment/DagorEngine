// an operator has no argument list, so piping a value into one is a compile error
options gen2
expect 30922:3

def side_effect : string {
    return "abc"
}

[export]
def main {
    var a = 5
    var s = "abc"
    debug(side_effect() |> !empty(s))
    debug(side_effect() |> -a)
    debug(side_effect() |> (a + a))
}
