options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT001: a statement after a `return` in the same block never runs; every dead statement of
// the block is reported.

expect 50503:3

require daslib/lint

def bad_after_return(x : int) : int {
    return x + 1
    print("never\n")               // LINT001
}

def bad_two_after_return(x : int) : int {
    return x
    let y = x + 1                  // LINT001 - every dead statement of the block is reported
    return y                       // LINT001
}

def good_conditional_return(x : int) : int {
    if (x > 0) {
        return x
    }
    return -x                      // reachable: the return above is under an if
}

[export]
def main() {
    print("{bad_after_return(1)} {bad_two_after_return(3)} {good_conditional_return(-2)}\n")
}
