options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF009 pin: a move-init that ends its block (nothing follows it) arms the
// pending slot and must produce NOTHING - not a false report against the next
// statement visited, whatever block or function that lands in. The two genuine
// var-then-return shapes below are the only expected findings.

expect 31208:2

require daslib/perf_lint

struct Thing {
    data : array<int>
}

def make_thing() : Thing {
    return <- Thing()
}

def arm_and_end_block(c : bool) : Thing {
    if (c) {
        var inscope orphan <- make_thing()
    }
    var inscope other <- make_thing()
    return <- other                         // PERF009 - genuine
}

def arm_at_function_end(c : bool) {
    var inscope tail_move <- make_thing()
}

def next_function_first_return() : Thing {
    var inscope fresh <- make_thing()
    return <- fresh                         // PERF009 - genuine
}
