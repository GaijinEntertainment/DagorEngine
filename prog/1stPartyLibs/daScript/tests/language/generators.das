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
    var gen <- generator<int>() <| $() {
        var t = 0
        while (t < 9) {
            yield t ++
        } finally {
            yield 9
        }
        return false
    }
    checkIt(t, gen)
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
    var gen <- generator<int>() <| $() {
        for (t in range(9)) {
            yield t
        } finally {
            yield 9
        }
        return false
    }
    checkIt(t, gen)
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
    t |> run("for loop in generator") @(t : T?) { test_for(t); }
    t |> run("for break in generator") @(t : T?) { test_for_break(t); }
    t |> run("for continue in generator") @(t : T?) { test_for_continue(t); }
    t |> run("nested for in generator") @(t : T?) { test_for_nested(t); }
    t |> run("for finally in generator") @(t : T?) { test_for_finally(t); }
}
