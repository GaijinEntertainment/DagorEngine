options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def bi_twice(b : block) {
    invoke(b)
    invoke(b)
}

def bi_twice_good(b : block<int>) : int {
    var tf = 0
    tf += invoke(b)
    tf += invoke(b)
    return tf
}

[sideeffects]
def bi_crapOnStack() {
    var a : int[20]
    for (b in a) {
        b = 13
    }
}

def bi_func1(b : block) {
    bi_crapOnStack()
    invoke(b)
    bi_crapOnStack()
}

def bi_func2(b : block) {
    bi_crapOnStack()
    bi_func3(b)
    bi_crapOnStack()
}

def bi_func3(b : block) {
    bi_crapOnStack()
    invoke(b)
    bi_crapOnStack()
}

def bi_func4(b : block) {
    bi_crapOnStack()
    invoke(b)
    bi_crapOnStack()
}

def bi_badd(ext : int; b : block<(arg : int) : int>) : int {
    return invoke(b, ext)
}

def bi_radd(var ext : int&; b : block<(var arg : int&) : int>) : int {
    return invoke(b, ext)
}

[test]
def test_block_invoke(t : T?) {
    t |> run("block variable invoke") @(t : T?) {
        var blk = $(a, b : int) {
            return a + b
        }
        t |> equal(invoke(blk, 1, 2), 3)
        t |> equal(invoke(blk, 3, 4), 7)
    }
    t |> run("twice and twice_good") @(t : T?) {
        var a = 0
        var b = 0
        bi_twice() ${ a++; }
        t |> equal(a, 2)
        b = bi_twice_good() <| ${ return a++; }
        t |> equal(a, 4)
        t |> equal(b, 2 + 3)
        b = bi_twice_good() <| $() {
            return a++
        }
        t |> equal(b, 4 + 5)
        b = bi_twice_good(${ return ++a; })
        t |> equal(a, 8)
        t |> equal(b, 7 + 8)
    }
    t |> run("nested block chaining") @(t : T?) {
        var x = 0
        var y = 0
        bi_func1() <| $() {
            x += 1
            y += 2
            bi_func2() <| $() {
                x++
                y += 3
            }
            bi_func4() <| $() {
                x++
                y += 4
            }
        }
        t |> equal(x, 3)
        t |> equal(y, 9)
    }
    t |> run("badd with value capture") @(t : T?) {
        var v1 = 1
        var res = bi_badd(1000, $(a1 : int) : int { return v1++ + a1; })
        t |> equal(res, 1001)
        res = bi_badd(1000) <| $(a1 : int) : int { return v1 + a1; }
        t |> equal(res, 1002)
        v1 = 3
        res = bi_badd(1000) <| $(a1 : int) : int {
            return v1 + a1
        }
        t |> equal(res, 1003)
    }
    t |> run("nested badd") @(t : T?) {
        var v1 = 4
        var res = bi_badd(1000) <| $(a2 : int) : int {
            return bi_badd(2000, $(a3 : int) : int { return a3 + a2 + v1; })
        }
        t |> equal(res, 3004)
    }
    t |> run("radd with ref") @(t : T?) {
        var v1 = 3
        var res = bi_radd(v1) <| $(var a : int&) : int {
            return a++
        }
        t |> equal(res, 3)
        t |> equal(v1, 4)
    }
}
