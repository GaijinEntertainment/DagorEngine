options gen2
// PERF030: move-assign onto a heap-carrying bare variable drops the old contents
//
// Problem: `a <- b` overwrites `a` without releasing what it held — a leak on a
// persistent heap. The inscope-pod rewrite heals the shape when force_inscope_pod
// is on (and marks the move it generates); every unhealed move warns.
//
// Bad pattern:
//   var a : array<int> ... a <- make_more()
//
// Good pattern:
//   delete a
//   a <- make_more()

expect 31208:4

require daslib/perf_lint

struct Holder {
    xs : array<int>
}

def make_arr : array<int> {
    var r : array<int>
    r |> push(1)
    return <- r
}

def bad_array_reassign {
    var a <- make_arr()
    a <- make_arr()          // PERF030: old contents dropped
    delete a
}

def bad_struct_reassign {
    var h : Holder
    h.xs |> push(1)
    var h2 : Holder
    h <- h2                  // PERF030: h.xs dropped
    delete h
}

def bad_table_reassign {
    var t : table<int; int>
    t |> insert(1, 2)
    var t2 : table<int; int>
    t <- t2                  // PERF030: t's buffer dropped
    delete t
}

def bad_fixed_array_reassign {
    var a : array<int>[2]
    a[0] |> push(1)
    var b : array<int>[2]
    a <- b                   // PERF030: the fixed array's element heap dropped
    delete a
}

def good_pod_move {
    var x = 1
    var y = 2
    // int carries no heap - never flagged (and <- on ints is not idiomatic anyway)
    x = y
}

def good_element_target {
    var t : table<int; array<int>>
    // element targets stay silent - dominated by fresh-slot inserts
    t[1] <- make_arr()
    delete t
}

def good_hoisted_inscope_init(flag : bool) : int {
    // the early-out relocation splits this decl into let + GENERATED move
    // (ast_allocate_stack.cpp RelocateVariables); the target is fresh and the
    // finally releases it - the lowering's move must stay silent
    return 0 if (flag)
    var inscope a <- make_arr()
    if (length(a) > 1) {
        return 2
    }
    return length(a)
}

[export]
def main {
    bad_array_reassign()
    bad_struct_reassign()
    bad_table_reassign()
    bad_fixed_array_reassign()
    good_pod_move()
    good_element_target()
    let _ = good_hoisted_inscope_init(false)
}
