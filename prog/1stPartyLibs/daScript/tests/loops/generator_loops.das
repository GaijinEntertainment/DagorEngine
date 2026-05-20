// Per-iteration finally in generator loops.
// Generators linearize loops to label/goto form (ast_generate.cpp: replaceGeneratorWhile /
// replaceGeneratorFor). Finally lowered to the iter_end / mid_loop label so yield-from-finally
// fires interleaved with body yields instead of once after the loop.
options gen2
require dastest/testing_boost public

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

[test]
def test_generator_loops(t : T?) {
    t |> run("for plain finally") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (i in range(3)) {
                yield i
            } finally {
                yield 99
            }
            return false
        }
        // body and finally interleaved per iteration
        checkSeq(t, gen, [0, 99, 1, 99, 2, 99])
    }
    t |> run("for finally break") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (i in range(10)) {
                yield i
                if (i == 1) {
                    break
                }
            } finally {
                yield 99
            }
            return false
        }
        // iter 0 body/finally, iter 1 body, break (finally still runs for this iter), exit
        checkSeq(t, gen, [0, 99, 1, 99])
    }
    t |> run("for finally continue") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (i in range(4)) {
                if (i == 1) {
                    continue
                }
                yield i
            } finally {
                yield 99
            }
            return false
        }
        // i=0 yield 0, finally 99
        // i=1 continue, finally 99
        // i=2 yield 2, finally 99
        // i=3 yield 3, finally 99
        checkSeq(t, gen, [0, 99, 99, 2, 99, 3, 99])
    }
    t |> run("for finally empty") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (i in range(0)) {
                yield i
            } finally {
                yield 99
            }
            return false
        }
        checkSeq(t, gen, [])
    }
    t |> run("while plain finally") @(t : T?) {
        var gen <- generator<int>() <| $() {
            var i = 0
            while (i < 3) {
                yield i
                i ++
            } finally {
                yield 99
            }
            return false
        }
        checkSeq(t, gen, [0, 99, 1, 99, 2, 99])
    }
    t |> run("while finally break") @(t : T?) {
        var gen <- generator<int>() <| $() {
            var i = 0
            while (i < 100) {
                yield i
                i ++
                if (i == 2) {
                    break
                }
            } finally {
                yield 99
            }
            return false
        }
        checkSeq(t, gen, [0, 99, 1, 99])
    }
    t |> run("while finally empty") @(t : T?) {
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
    t |> run("nested for finally") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (y in range(2)) {
                for (x in range(2)) {
                    yield y * 10 + x
                } finally {
                    yield 77  // inner finally per inner iter
                }
            } finally {
                yield 99  // outer finally per outer iter
            }
            return false
        }
        // y=0:
        //   x=0 yield 0, inner finally 77
        //   x=1 yield 1, inner finally 77
        //   outer finally 99
        // y=1:
        //   x=0 yield 10, inner finally 77
        //   x=1 yield 11, inner finally 77
        //   outer finally 99
        checkSeq(t, gen, [0, 77, 1, 77, 99, 10, 77, 11, 77, 99])
    }
    t |> run("for finally return") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (i in range(5)) {
                yield i
                if (i == 1) {
                    return false   // early generator end — iter 1's finally must fire first
                }
            } finally {
                yield 99
            }
            return false
        }
        // iter 0: yield 0, finally 99
        // iter 1: yield 1, return triggers finally (yield 99), then generator ends
        checkSeq(t, gen, [0, 99, 1, 99])
    }
    t |> run("while finally return") @(t : T?) {
        var gen <- generator<int>() <| $() {
            var i = 0
            while (i < 10) {
                yield i
                if (i == 2) {
                    return false
                }
                i ++
            } finally {
                yield 99
            }
            return false
        }
        // iter i=0: yield 0, finally 99
        // iter i=1: yield 1, finally 99
        // iter i=2: yield 2, return triggers finally (yield 99), then generator ends
        checkSeq(t, gen, [0, 99, 1, 99, 2, 99])
    }
    t |> run("for finally return nested") @(t : T?) {
        var gen <- generator<int>() <| $() {
            for (y in range(3)) {
                for (x in range(3)) {
                    yield y * 10 + x
                    if (x == 1 && y == 1) {
                        return false
                    }
                } finally {
                    yield 77   // inner finally must fire per inner iter + on return
                }
            } finally {
                yield 99   // outer finally must fire on the return path too
            }
            return false
        }
        // y=0: x=0 yield 0, 77; x=1 yield 1, 77; x=2 yield 2, 77; outer 99
        // y=1: x=0 yield 10, 77; x=1 yield 11, return -> inner 77, outer 99, generator ends
        checkSeq(t, gen, [0, 77, 1, 77, 2, 77, 99, 10, 77, 11, 77, 99])
    }
}
