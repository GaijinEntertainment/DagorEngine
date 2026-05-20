// Per-iteration finally correctness for nested loops of mixed kinds.
options gen2
require dastest/testing_boost public

[test]
def test_nested(t : T?) {
    t |> run("for in for") @(t : T?) {
        var outer = 0
        var inner = 0
        for (_y in range(4)) {
            for (_x in range(3)) {
            } finally {
                inner ++
            }
        } finally {
            outer ++
        }
        t |> equal(outer, 4)
        t |> equal(inner, 4 * 3)
    }
    t |> run("while in for") @(t : T?) {
        var outer = 0
        var inner = 0
        for (_y in range(3)) {
            var x = 0
            while (x < 5) {
                x ++
            } finally {
                inner ++
            }
        } finally {
            outer ++
        }
        t |> equal(outer, 3)
        t |> equal(inner, 3 * 5)
    }
    t |> run("for in while") @(t : T?) {
        var y = 0
        var outer = 0
        var inner = 0
        while (y < 3) {
            for (_x in range(4)) {
            } finally {
                inner ++
            }
            y ++
        } finally {
            outer ++
        }
        t |> equal(outer, 3)
        t |> equal(inner, 3 * 4)
    }
    t |> run("triple nested") @(t : T?) {
        var outer = 0
        var mid = 0
        var inner = 0
        for (_z in range(2)) {
            for (_y in range(3)) {
                for (_x in range(4)) {
                } finally {
                    inner ++
                }
            } finally {
                mid ++
            }
        } finally {
            outer ++
        }
        t |> equal(outer, 2)
        t |> equal(mid, 2 * 3)
        t |> equal(inner, 2 * 3 * 4)
    }
    t |> run("inner break preserves outer") @(t : T?) {
        var outer = 0
        var inner = 0
        for (_y in range(3)) {
            for (x in range(10)) {
                if (x == 2) {
                    break
                }
            } finally {
                inner ++
            }
        } finally {
            outer ++
        }
        // inner break at x==2: 3 finally fires per outer iter; outer runs full 3 times
        t |> equal(outer, 3)
        t |> equal(inner, 3 * 3)
    }
    t |> run("outer break after inner finishes") @(t : T?) {
        var outer = 0
        var inner = 0
        for (y in range(5)) {
            for (_x in range(2)) {
            } finally {
                inner ++
            }
            if (y == 1) {
                break
            }
        } finally {
            outer ++
        }
        // outer breaks on y==1, so outer iterations 0 and 1 fire: outer finally = 2
        t |> equal(outer, 2)
        t |> equal(inner, 2 * 2)
    }
}
