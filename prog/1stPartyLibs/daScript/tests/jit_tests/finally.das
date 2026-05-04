options gen2
require dastest/testing_boost

[test]
def test_finally(t : T?) {
    t |> run("test if finally") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_if_finally))
        t |> success(test_if_finally())
    }
    t |> run("test nested if") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_nested_if))
        t |> success(test_nested_if())
    }
    t |> run("test nested if else") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_nested_if_else))
        t |> success(test_nested_if_else())
    }
    t |> run("test if doulbe break") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_if_double_break))
        t |> success(test_if_double_break())
    }
    t |> run("test for finally") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_for_finally))
        t |> success(test_for_finally())
    }
    t |> run("test while finally") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_while_finally))
        t |> success(test_while_finally())
    }
    t |> run("test func finally") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_func_finally))
        t |> success(test_func_finally())
    }
    t |> run("test block finally") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_block_finally))
        t |> success(test_block_finally())
    }
    t |> run("for loop with early out") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@early_out))
        g_steps = 0
        early_out(t, array<int>(1, 2, 3, 4))
        t |> equal(2, g_steps)
    }
}

var g_steps = 0

[jit, sideeffects]
def early_out(t : T?; var aclone : array<int>) {
    t |> equal(0, lock_count(aclone))
    for (aidx in aclone) {
        if (aidx == 2) {
            t |> equal(1, lock_count(aclone))
            return
        }
    } finally {
        g_steps ++
        t |> equal(0, lock_count(aclone))
    }
} finally {
    t |> equal(0, lock_count(aclone))
    g_steps ++
    delete aclone
}

[jit, sideeffects]
def test_if_double_break(max : int = 10) {
    var count = 0
    for (x in range(max)) {
        if (x == 1) {
            continue
        } finally {
            count += 10
        }
        if (x == 2) {
            continue
        } finally {
            count += 100
        }
        count ++
    }
    return count == 118
}

[jit, sideeffects]
def test_nested_if(max : int = 10) {
    var t = 0
    for (x in range(max)) {
        if (x == 5) {
            if (x > 4) {
                break
            } finally {
                t += 1
            }
        } finally {
            t += 10
        }
    }
    return t == 11
}

[jit, sideeffects]
def test_nested_if_else(max : int = 10) {
    var t = 0
    var p = false
    for (x in range(max)) {
        if (x == 5) {
            if (x < 4) {
                p = true
            } else {
                break
            } finally {
                t += 1
            }
        } finally {
            t += 10
        }
    }
    return t == 11 && p == false
}

[jit, sideeffects]
def test_if_finally(max : int = 10) {
    var t : int[10]
    var count = 0
    for (x in range(max)) {
        t[x] = x + 1
        if (x == 5) {
            break
        } finally {
            t[9] = 13
            count ++
        }
    }
    return count == 1 && t[9] == 13
}

[jit, sideeffects]
def test_for_finally(max : int = 10) {
    var t : int[10]
    var count = 0
    for (x in range(max)) {
        let uno = one()
        t[x] = x + uno
        if (x == 5) {
            break
        }
    } finally {
        t[9] = 13
        count += uno
    }
    return count == 1 && t[9] == 13
}

var {
    ONE = 1
}

[jit, sideeffects]
def one {
    ONE = 1
    assert(ONE == 1)
    return ONE
}

[jit, sideeffects]
def test_while_finally(max : int = 10) {
    var t : int[10]
    var count = 0
    var x = 0
    while (x != max) {
        let uno = one()
        t[x] = x + uno
        if (x == 5) {
            break
        }
        ++x
    } finally {
        t[9] = 12 + uno
        count ++
    }
    return count == 1 && t[9] == 13
}

var {
    glob = 0
    glob_count = 0
}

[jit, sideeffects]
def func_finally {
    return 12
} finally {
    glob = 13
    glob_count ++
}

[jit, sideeffects]
def test_func_finally() {
    glob = 0
    glob_count = 0
    let t = func_finally()
    return t == 12 && glob == 13 && glob_count == 1
}

[jit, sideeffects]
def test_block_finally() {
    var t = 0
    invoke() <| $() { t = 1; } finally { t = 2; }
    return t == 2
}


