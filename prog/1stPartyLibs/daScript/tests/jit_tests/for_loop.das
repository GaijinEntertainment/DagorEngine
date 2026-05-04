options gen2
require dastest/testing_boost

[jit]
def string_for(t : T?) {
    for (c, i in "hello", count()) {
        if (i == 0) {
            t |> equal(c, 'h')
        } elif (i == 1) {
            t |> equal(c, 'e')
        } elif (i == 2 || i == 3) {
            t |> equal(c, 'l')
        } elif (i == 4) {
            t |> equal(c, 'o')
        } else {
            t |> failure("unexpected character")
        }
    }
}

[jit]
def range_for(value : int) {
    var i = 0
    for (_ in range(0, value)) {
        i ++
    }
    return i
}

[jit]
def range_for_range(value : range) {
    var i = 0
    for (_ in value) {
        i ++
    }
    return i
}

[jit]
def range_for_range_extra(value : range) {
    var i = 0
    for (a, b in value, range(13, 100500)) {
        i ++
    }
    return i
}

[jit]
def range_for_with_break(value, max_value : int) {
    var i = 0
    for (_ in range(0, value)) {
        if (i >= max_value) {
            break
        }
        i ++
    }
    return i
}

[jit]
def range_for_with_continue(value, max_value : int) {
    var i = 0
    var j = 0
    for (_ in range(0, value)) {
        i ++
        if (i >= max_value) {
            continue
        }
        j ++
    }
    return j
}

[jit]
def range_for_with_finally(value : int) {
    var i = 0
    var tf = 0
    for (_ in range(0, value)) {
        i ++
    } finally {
        tf ++
    }
    return tf
}

[jit]
def dim_for(var a : int[10]) {
    var count = 0
    for (j in a) {
        j = 13
        count ++
    }
    return count
}

[jit]
def dim_and_range_for(var a : int[10]) {
    var count = 0
    for (i, j in range(5), a) {
        j = i
        count ++
    }
    return count
}

[jit]
def array_for(var a : array<int>) {
    var count = 0
    for (j in a) {
        j = 13
        count ++
    }
    return count
}

[jit]
def array_and_range_for(var a : array<int>) {
    var count = 0
    for (i, j in range(5), a) {
        j = i
        count ++
    }
    return count
}

def gen_range(value : int) {
    return <- generator<int>() <| $() {
        for (i in range(value)) {
            yield i
        }
        return false
    }
}

[jit]
def iterator_for(value : int) {
    var i = 0
    for (_ in gen_range(value)) {
        i ++
    }
    return i
}

[jit]
def hybrid_for(value : int) {
    var i = 0
    for (a, b in gen_range(value), range(7)) {
        i ++
    }
    return i
}

[jit]
def array_comp(value : int) {
    return <- [for (x in range(0, value)); x]
}

[test]
def test_comprehension(t : T?) {
    t |> run("comprehension") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@array_comp))
        var a <- array_comp(10)
        for (x, y in a, range(10)) {
            t |> equal(x, y)
        }
    }
}

[jit]
def add_elements(a : array<int>) {
    var summ = 0
    for (i in a) {
        summ += i
    }
    return summ
}

[jit]
def special_count(max, start, step : int) {
    var add = 0
    for (i, j in range(max), count(start, step)) {
        add += j
    }
    return add
}

[test]
def test_for_loop(t : T?) {
    t |> run("string for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@string_for))
        string_for(t)
    }
    t |> run("range for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@range_for))
        t |> success(!jit_enabled() || is_jit_function(@@range_for_range))
        t |> success(!jit_enabled() || is_jit_function(@@range_for_range_extra))
        t |> success(!jit_enabled() || is_jit_function(@@range_for_with_break))
        t |> success(!jit_enabled() || is_jit_function(@@range_for_with_continue))
        t |> success(!jit_enabled() || is_jit_function(@@range_for_with_finally))
        t |> equal(range_for(10), 10)
        t |> equal(range_for_range(range(100, 110)), 10)
        t |> equal(range_for_range_extra(range(100, 110)), 10)
        t |> equal(range_for_with_break(10, 5), 5)
        t |> equal(range_for_with_continue(10, 5), 4)
        t |> equal(range_for_with_finally(10), 1)
    }
    t |> run("dim for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@dim_for))
        t |> success(!jit_enabled() || is_jit_function(@@dim_and_range_for))
        var a : int[10]
        t |> equal(dim_for(a), 10)
        for (i in a) {
            t |> equal(i, 13)
        }
        t |> equal(dim_and_range_for(a), 5)
        for (i, j in a, range(10)) {
            t |> equal(i, j < 5 ? j : 13)
        }
    }
    t |> run("array for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@array_for))
        t |> success(!jit_enabled() || is_jit_function(@@array_and_range_for))
        var a : array<int>
        a |> resize(10)
        t |> equal(array_for(a), 10)
        for (i in a) {
            t |> equal(i, 13)
        }
        t |> equal(array_and_range_for(a), 5)
        for (i, j in a, range(10)) {
            t |> equal(i, j < 5 ? j : 13)
        }
    }
    t |> run("iterator for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@iterator_for))
        t |> equal(iterator_for(10), 10)
    }
    t |> run("hybrid for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@hybrid_for))
        t |> equal(7, hybrid_for(10))
        t |> equal(5, hybrid_for(5))
    }
    t |> run("empty for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@add_elements))
        var a <- array<int>(1, 2, 3)
        t |> equal(6, add_elements(a))
        a |> clear()
        t |> equal(0, add_elements(a))
    }
    t |> run("special count") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@special_count))
        t |> equal(3,  special_count(3, 0, 1))  // 0, 1, 2
        t |> equal(6,  special_count(3, 0, 2))  // 0, 2, 4
        t |> equal(12, special_count(3, 1, 3))  // 1, 4, 7
    }
}

