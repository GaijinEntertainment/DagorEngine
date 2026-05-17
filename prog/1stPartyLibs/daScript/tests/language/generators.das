// Tests raw generator<T>() mechanics: yield, control flow, locals, refs
options gen2
require dastest/testing_boost public

[sideeffects]
def checkIt(t : T?; var gen : iterator<int>) {
    var count = 0
    for (x, y in gen, count()) {
        if (y >= 10) {
            t |> failure("output too long")
        }
        t |> equal(x, y)
        count ++
    }
    t |> equal(count, 10)
}

[sideeffects]
def checkSeq(t : T?; var gen : iterator<int>; expected : array<int>) {
    var i = 0
    for (x in gen) {
        if (i >= length(expected)) {
            t |> failure("output too long")
            return
        }
        t |> equal(x, expected[i])
        i ++
    }
    t |> equal(i, length(expected))
}

def test_yield(t : T?) {
    var gen <- generator<int>() <| $() {
        yield 0
        yield 1
        yield 2
        yield 3
        yield 4
        yield 5
        yield 6
        yield 7
        yield 8
        yield 9
        return false
    }
    checkIt(t, gen)
}

def test_local(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        return false
    }
    checkIt(t, gen)
}

def test_local_ref(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        var T : int& = t
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        return false
    }
    checkIt(t, gen)
}

def test_if(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        yield t++
        if (t == 1) {
            yield t++
        }
        if (t == 2) {
            yield t++
        } else {
            yield 13
        }
        if (t == 3) {
            if (t < 10) {
                yield t++
            } else {
                yield 13
            }
        } else {
            yield 12
        }
        if (t == 13) {
            yield 0
        } elif (t == 4) {
            yield t++
        } else {
            yield -1
        }
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        return false
    }
    checkIt(t, gen)
}

def test_while(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 10) {
            yield t ++
        }
        return false
    }
    checkIt(t, gen)
}

def test_while_break(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 100) {
            if (t == 10) {
                break
            }
            yield t ++
        }
        return false
    }
    checkIt(t, gen)
}

def test_while_continue(t : T?) {
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 100) {
            if (t >= 10) {
                t ++
                continue
            }
            yield t ++
        }
        return false
    }
    checkIt(t, gen)
}

def test_while_nested(t : T?) {
    var gen <- generator<int>() <| $() {
        var q = 0
        while (q < 2) {
            var t = 0
            while (t < 5) {
                yield q * 5 + t ++
            }
            q ++
        }
        return false
    }
    checkIt(t, gen)
}

def test_while_finally(t : T?) {
    // per-iteration semantics: finally runs at the end of every iteration of the body,
    // NOT once after the loop. `yield 9` fires interleaved after each body yield.
    // cond becomes false -> finally does NOT run for that phantom iteration.
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 9) {
            yield t ++
        } finally {
            yield 9
        }
        return false
    }
    checkSeq(t, gen, [0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9])
}

def test_while_finally_break(t : T?) {
    // break routes through finally once (current iteration's finally), then exits
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 100) {
            yield t ++
            if (t == 3) {
                break
            }
        } finally {
            yield 99
        }
        return false
    }
    // iter 0: yield 0 -> t=1, no break, finally yields 99
    // iter 1: yield 1 -> t=2, no break, finally yields 99
    // iter 2: yield 2 -> t=3, break, finally yields 99, exit
    checkSeq(t, gen, [0, 99, 1, 99, 2, 99])
}

def test_while_finally_continue(t : T?) {
    // continue routes through finally (same as normal end of iter), then loops back
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 5) {
            t ++
            if (t % 2 == 0) {
                continue
            }
            yield t
        } finally {
            yield 99
        }
        return false
    }
    // t=1: odd, yield 1, finally 99
    // t=2: even, continue, finally 99
    // t=3: odd, yield 3, finally 99
    // t=4: even, continue, finally 99
    // t=5: odd, yield 5, finally 99
    // t<5 false, exit without finally
    checkSeq(t, gen, [1, 99, 99, 3, 99, 99, 5, 99])
}

def test_while_finally_empty(t : T?) {
    // empty loop: body never enters, finally never runs
    var gen <- generator<int>() <| $() {
        while (false) {
            yield 1
        } finally {
            yield 99
        }
        return false
    }
    checkSeq(t, gen, [])
}

def test_for(t : T?) {
    var gen <- generator<int>() <| $() {
        for (t in range(10)) {
            yield t
        }
        return false
    }
    checkIt(t, gen)
}

def test_for_break(t : T?) {
    var gen <- generator<int>() <| $() {
        for (t in range(100)) {
            if (t == 10) {
                break
            }
            yield t
        }
        return false
    }
    checkIt(t, gen)
}

def test_for_continue(t : T?) {
    var gen <- generator<int>() <| $() {
        for (t in range(100)) {
            if (t >= 10) {
                continue
            }
            yield t
        }
        return false
    }
    checkIt(t, gen)
}

def test_for_nested(t : T?) {
    var gen <- generator<int>() <| $() {
        for (q in range(2)) {
            for (t in range(5)) {
                yield q * 5 + t
            }
        }
        return false
    }
    checkIt(t, gen)
}

def test_for_finally(t : T?) {
    // per-iteration semantics: finally runs at mid_loop (after body, before iterator_next)
    // for N iterations we get N body yields and N finally yields, interleaved.
    // On iterator exhaustion: initial !loop check jumps past finally (skipped for empty or post-last).
    var gen <- generator<int>() <| $() {
        for (t in range(9)) {
            yield t
        } finally {
            yield 9
        }
        return false
    }
    checkSeq(t, gen, [0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9])
}

def test_for_finally_break(t : T?) {
    var gen <- generator<int>() <| $() {
        for (t in range(100)) {
            yield t
            if (t == 2) {
                break
            }
        } finally {
            yield 99
        }
        return false
    }
    // iter 0: yield 0, t!=2, finally 99
    // iter 1: yield 1, t!=2, finally 99
    // iter 2: yield 2, t==2 break, finally 99, exit
    checkSeq(t, gen, [0, 99, 1, 99, 2, 99])
}

def test_for_finally_continue(t : T?) {
    var gen <- generator<int>() <| $() {
        for (t in range(5)) {
            if (t % 2 == 0) {
                continue
            }
            yield t
        } finally {
            yield 99
        }
        return false
    }
    // t=0 even continue, finally 99
    // t=1 odd, yield 1, finally 99
    // t=2 even continue, finally 99
    // t=3 odd, yield 3, finally 99
    // t=4 even continue, finally 99
    checkSeq(t, gen, [99, 1, 99, 99, 3, 99, 99])
}

def test_for_finally_empty(t : T?) {
    // empty iterator: body never enters, finally never runs
    var gen <- generator<int>() <| $() {
        for (t in range(0)) {
            yield t
        } finally {
            yield 99
        }
        return false
    }
    checkSeq(t, gen, [])
}

[test]
def test_generators(t : T?) {
    t |> run("yield explicit values") @(t : T?) { test_yield(t); }
    t |> run("local variable in generator") @(t : T?) { test_local(t); }
    t |> run("local ref in generator") @(t : T?) { test_local_ref(t); }
    t |> run("if/elif/else in generator") @(t : T?) { test_if(t); }
    t |> run("while loop in generator") @(t : T?) { test_while(t); }
    t |> run("while break in generator") @(t : T?) { test_while_break(t); }
    t |> run("while continue in generator") @(t : T?) { test_while_continue(t); }
    t |> run("nested while in generator") @(t : T?) { test_while_nested(t); }
    t |> run("while finally in generator") @(t : T?) { test_while_finally(t); }
    t |> run("while finally + break in generator") @(t : T?) { test_while_finally_break(t); }
    t |> run("while finally + continue in generator") @(t : T?) { test_while_finally_continue(t); }
    t |> run("while finally empty in generator") @(t : T?) { test_while_finally_empty(t); }
    t |> run("for loop in generator") @(t : T?) { test_for(t); }
    t |> run("for break in generator") @(t : T?) { test_for_break(t); }
    t |> run("for continue in generator") @(t : T?) { test_for_continue(t); }
    t |> run("nested for in generator") @(t : T?) { test_for_nested(t); }
    t |> run("for finally in generator") @(t : T?) { test_for_finally(t); }
    t |> run("for finally + break in generator") @(t : T?) { test_for_finally_break(t); }
    t |> run("for finally + continue in generator") @(t : T?) { test_for_finally_continue(t); }
    t |> run("for finally empty in generator") @(t : T?) { test_for_finally_empty(t); }
}
