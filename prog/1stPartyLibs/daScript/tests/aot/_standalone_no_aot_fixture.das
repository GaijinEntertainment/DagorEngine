options gen2

[no_aot]
def interpreted_helper(x : int) : int {
    return x * 3
}

[export]
def calls_interpreted_helper(x : int) : int {
    return interpreted_helper(x) + 1
}
