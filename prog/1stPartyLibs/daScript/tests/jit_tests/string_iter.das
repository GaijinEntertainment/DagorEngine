options gen2
require dastest/testing_boost

struct Box {
    arr : array<int>
}

def sum_chars(s : string) : int {
    var acc = 0
    for (c in s) {
        acc += c
    }
    return acc
}

def collect(s : string) : array<int> {
    var res : array<int>   // nolint:STYLE027 - the push loop IS the path under test
    for (c in s) {
        res |> push(c)   // nolint:PERF006 - growing from empty is the path under test
    }
    return <- res
}

[test]
def test_string_iteration(t : T?) {
    t |> run("empty string never enters the body") @(t : T?) {
        var n = 0
        for (_c in "") {
            n++
        }
        t |> equal(0, n)
        t |> equal(0, sum_chars(""))
    }
    t |> run("single char") @(t : T?) {
        t |> equal(65, sum_chars("A"))
        let v <- collect("A")
        t |> equal(1, length(v))
        t |> equal(65, v[0])
    }
    t |> run("ascii run") @(t : T?) {
        t |> equal(104 + 105, sum_chars("hi"))
        let v <- collect("abc")
        t |> equal(3, length(v))
        t |> equal(97, v[0])
        t |> equal(98, v[1])
        t |> equal(99, v[2])
    }
    t |> run("high bytes stay unsigned") @(t : T?) {
        let v <- collect("é")
        t |> equal(2, length(v))
        t |> equal(195, v[0])
        t |> equal(169, v[1])
        t |> equal(195 + 169, sum_chars("é"))
    }
    t |> run("0xff byte") @(t : T?) {
        let v <- collect("\xff")
        t |> equal(1, length(v))
        t |> equal(255, v[0])
    }
    t |> run("break stops early") @(t : T?) {
        var n = 0
        for (c in "abcdef") {
            n++
            if (c == 'c') {
                break
            }
        }
        t |> equal(3, n)
    }
    t |> run("continue skips") @(t : T?) {
        var acc = 0
        for (c in "abc") {
            if (c == 'b') {
                continue
            }
            acc += c
        }
        t |> equal(97 + 99, acc)
    }
    t |> run("variable source, not just a literal") @(t : T?) {
        let s = "hello"
        t |> equal(sum_chars(s), sum_chars("hello"))
        var total = 0
        for (c in s) {
            total += c
        }
        t |> equal(sum_chars("hello"), total)
    }
    t |> run("nested loops over two strings") @(t : T?) {
        var pairs = 0
        for (_a in "ab") {
            for (_b in "xyz") {
                pairs++
            }
        }
        t |> equal(6, pairs)
    }
    t |> run("loop twice over the same string") @(t : T?) {
        let s = "abc"
        t |> equal(sum_chars(s), sum_chars(s))
    }
}

[test]
def test_string_iteration_multi_source(t : T?) {
    t |> run("multi-source stops at the shortest") @(t : T?) {
        var arr <- [1, 2, 3, 4, 5]
        var n = 0
        for (x, _c in arr, "ab") {
            n += x
        }
        t |> equal(1 + 2, n)
    }
    t |> run("empty companion source keeps the loop out") @(t : T?) {
        var empty : array<int>
        var n = 0
        for (x, _c in empty, "abc") {
            n = n * 100 + x
        }
        t |> equal(0, n)
    }
    t |> run("empty string keeps a companion array out and unlocked") @(t : T?) {
        var arr <- [1, 2, 3]
        var n = 0
        for (_c, x in "", arr) {
            n += x
        }
        t |> equal(0, n)
        arr |> push(4)
        t |> equal(4, length(arr))
    }
    t |> run("each() over a field beside a string source") @(t : T?) {
        let b <- Box(arr <- [1, 2, 3])
        var n = 0
        for (_c, x in "ab", each(b.arr)) {
            n += x
        }
        t |> equal(1 + 2, n)
    }
}
