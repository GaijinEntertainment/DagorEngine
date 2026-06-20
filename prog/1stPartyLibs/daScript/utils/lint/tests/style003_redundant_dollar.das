options gen2
// STYLE003: redundant $() on parameterless block argument
//
// Problem: $() is unnecessary when the block takes no parameters.
//
// Bad pattern:
//   takes_block() $() { print("done\n") }
//
// Good pattern:
//   takes_block() { print("done\n") }

def takes_block(blk : block) {
    invoke(blk)
}

// bad: redundant $() (1 warning)
def bad_redundant_dollar() {
    takes_block() $() {
        print("done\n")
    }
}

// bad: standalone block literal with redundant $() (1 warning)
def bad_standalone_dollar() {
    let blk = $() {
        print("done\n")
    }
    invoke(blk)
}

// good: bare block
def good_bare_block() {
    takes_block() {
        print("done\n")
    }
}

// good: standalone block without redundant $()
def good_standalone_dollar() {
    let blk = $ {
        print("done\n")
    }
    invoke(blk)
}

// good: block with params — $() is required
def good_with_params() {
    var x = 0
    takes_int_block() $(a) {
        x = a
    }
}

def takes_int_block(blk : block<(a : int) : void>) {
    invoke(blk, 42)
}
