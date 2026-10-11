options gen2
require dastest/testing_boost public

[test]
def test_basic_bare_block(t : T?) {
    t |> run("bare block executes statements") @(t : T?) {
        var x = 0
        {
            x = 42
        }
        t |> equal(x, 42)
    }
    t |> run("bare block with multiple statements") @(t : T?) {
        var a = 0
        var b = 0
        {
            a = 1
            b = 2
        }
        t |> equal(a, 1)
        t |> equal(b, 2)
    }
    t |> run("bare block scopes variables") @(t : T?) {
        var x = 10
        {
            let y = 20
            x = x + y
        }
        t |> equal(x, 30)
    }
}

[test]
def test_nested_bare_blocks(t : T?) {
    t |> run("nested bare blocks") @(t : T?) {
        var result = 0
        {
            let a = 1
            {
                let b = 2
                result = a + b
            }
        }
        t |> equal(result, 3)
    }
    t |> run("deeply nested bare blocks") @(t : T?) {
        var result = 0
        {
            {
                {
                    result = 99
                }
            }
        }
        t |> equal(result, 99)
    }
}

[test]
def test_bare_block_sequential(t : T?) {
    t |> run("multiple sequential bare blocks") @(t : T?) {
        var x = 0
        {
            x += 10
        }
        {
            x += 20
        }
        {
            x += 30
        }
        t |> equal(x, 60)
    }
}

[test]
def test_bare_block_in_control_flow(t : T?) {
    t |> run("bare block inside if") @(t : T?) {
        var x = 0
        if (true) {     // nolint:STYLE010 - the always-taken if is the container this arm nests a bare block inside
            {
                x = 42
            }
        }
        t |> equal(x, 42)
    }
    t |> run("bare block inside for") @(t : T?) {
        var sum = 0
        for (i in range(3)) {
            {
                sum += i
            }
        }
        t |> equal(sum, 3)
    }
    t |> run("bare block inside while") @(t : T?) {
        var count = 0
        var n = 3
        while (n > 0) {
            {
                count ++
                n --
            }
        }
        t |> equal(count, 3)
    }
}

[test]
def test_bare_block_with_finally(t : T?) {
    t |> run("bare block with finally clause") @(t : T?) {
        var x = 0
        var fin_ran = false
        {
            x = 42
        } finally {
            fin_ran = true
        }
        t |> equal(x, 42)
        t |> success(fin_ran)
    }
}

[test]
def test_bare_block_scope(t : T?) {
    t |> run("inner var not visible outside") @(t : T?) {
        var x = 1
        {
            let y = 2
            x += y
        }
        // y is no longer in scope here — only x survives
        t |> equal(x, 3)
    }
    t |> run("inner block modifies outer var") @(t : T?) {
        var x = 10
        {
            let inner_val = 99
            x = inner_val
        }
        // inner_val no longer in scope, but x was modified
        t |> equal(x, 99)
    }
    t |> run("nested blocks each have own scope") @(t : T?) {
        var result = 0
        {
            let a = 1
            {
                let b = 2
                {
                    let c = 3
                    result = a + b + c
                }
                // c not visible here
                t |> equal(a + b, 3)
            }
            // b not visible here
            t |> equal(a, 1)
        }
        // a not visible here
        t |> equal(result, 6)
    }
    t |> run("sequential blocks reuse names") @(t : T?) {
        var sum = 0
        {
            let tmp = 10
            sum += tmp
        }
        {
            let tmp = 20
            sum += tmp
        }
        {
            let tmp = 30
            sum += tmp
        }
        t |> equal(sum, 60)
    }
    t |> run("bare block var lifetime ends at block end") @(t : T?) {
        var arr : array<int>
        {
            var local_arr = [1, 2, 3]
            arr |> push_from(local_arr)
        }
        t |> equal(length(arr), 3)
        t |> equal(arr[0], 1)
        t |> equal(arr[2], 3)
    }
}

[test]
def test_bare_block_name_reuse(t : T?) {
    t |> run("same name different type in sequential blocks") @(t : T?) {
        var ok = true
        {
            var a : int = 1
            a++
            ok = ok && a == 2
        }
        {
            let a : string = "hello"
            ok = ok && a == "hello"
        }
        t |> success(ok)
    }
    t |> run("same name in block then outer scope") @(t : T?) {
        var result = ""
        {
            let a = "inner"
            result = a
        }
        let a = "outer"
        t |> equal(result, "inner")
        t |> equal(a, "outer")
    }
    t |> run("same name in nested block then enclosing scope") @(t : T?) {
        var n1 = 0
        var n2 = 0
        {
            {
                let a = 10
                n1 = a
            }
            let a = 20
            n2 = a
        }
        t |> equal(n1, 10)
        t |> equal(n2, 20)
    }
    t |> run("three sequential blocks reuse name with different types") @(t : T?) {
        var sum = 0
        {
            let v = 1
            sum += v
        }
        {
            let v = 2.0
            sum += int(v)
        }
        {
            let v = 3
            sum += v
        }
        t |> equal(sum, 6)
    }
}
