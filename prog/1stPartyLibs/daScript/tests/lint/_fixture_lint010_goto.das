options gen2

// LINT010 fixture: every store below a goto or a label is read at the jump target, so none is
// dead - the overwrite under a label, and the one on a variable declared under it, would both
// report without the stand-down.

def forward_goto(n : int) : int {
    var res = 1
    if (n > 0) {
        goto label 1
    }
    res = 2
    label 1:
    return res
}

def computed_goto(n : int) : int {
    var res = -1
    goto n
    label 0:
    res = 10
    goto label 2
    label 1:
    res = 11
    goto label 2
    label 2:
    return res
}

def store_before_backward_label(n : int) : int {
    var acc = 0
    var i = n
    label 0:
    acc = i
    i--
    if (i > 0) {
        goto label 0
    }
    return acc
}

def declared_under_label(n : int) : int {
    var acc = 0
    label 0:
    var late = 1
    late = n
    acc += late
    if (acc < 0) {
        goto label 0
    }
    return acc
}

def genuine_dead_store(x : int) : int {
    var t = 0
    t = x * 2
    return t
}
