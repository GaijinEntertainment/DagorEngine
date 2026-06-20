options gen2
// STYLE002: <| pipe before parameterless block
//
// Problem: <| pipe and $() are both unnecessary for parameterless blocks.
//
// Bad pattern:
//   takes_block() <| $() { print("done\n") }
//
// Good pattern:
//   takes_block() { print("done\n") }

def takes_block(blk : block) {
    invoke(blk)
}

// bad: pipe and $() (1 warning)
def bad_pipe_empty_block() {
    takes_block() <| $() {
        print("done\n")
    }
}

// good: bare block
def good_bare_block() {
    takes_block() {
        print("done\n")
    }
}
