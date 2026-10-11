options gen2
require dastest/testing_boost

def each_array_sum(a : array<int>) : int {
    var s = 0
    for (x in each(a)) {
        s += x
    }
    return s
}

def each_dim_sum(a : int[4]) : int {
    var s = 0
    for (x in each(a)) {
        s += x
    }
    return s
}

def each_array_bump(var a : array<int>) {   // nolint:LINT014 - writes go through the element reference
    for (x in each(a)) {
        x++
    }
}

def each_break_at(a : array<int>; stop : int) : int {
    var n = 0
    for (_x in each(a)) {
        n++
        break if (n == stop)
    }
    return n
}

def each_return_at(a : array<int>; stop : int) : int {
    var n = 0
    for (x in each(a)) {
        n++
        return x if (n == stop)
    }
    return -1
}

def each_with_range(a : array<int>) : int {
    var s = 0
    for (x, i in each(a), range(2)) {
        s += x * (i + 1)
    }
    return s
}

def each_nested(a : array<int>) : int {
    var s = 0
    for (x in each(a)) {
        for (y in each(a)) {
            s += x * y
        }
    }
    return s
}

def each_table_stays_generic(t : table<int; int>) : int {
    var s = 0
    for (kv in each_kv(t)) {
        s += kv.key + kv.value
    }
    return s
}

[test]
def test_each_inline(t : T?) {
    t |> run("array source walks inline") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@each_array_sum))
        var a <- [for (i in range(5)); i]
        t |> equal(each_array_sum(a), 10)
        delete a
    }
    t |> run("fixed array source walks inline") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@each_dim_sum))
        var d : int[4]
        for (i in range(4)) {
            d[i] = i * 10
        }
        t |> equal(each_dim_sum(d), 60)
    }
    t |> run("element is a writable reference") @(t : T?) {
        var a <- [for (i in range(4)); i]
        each_array_bump(a)
        t |> equal(each_array_sum(a), 10)
        delete a
    }
    t |> run("break unlocks the array") @(t : T?) {
        var a <- [for (i in range(5)); i]
        t |> equal(each_break_at(a, 3), 3)
        a |> push(5)
        t |> equal(length(a), 6)
        delete a
    }
    t |> run("return unlocks the array") @(t : T?) {
        var a <- [for (i in range(5)); i]
        t |> equal(each_return_at(a, 4), 3)
        a |> push(5)
        t |> equal(length(a), 6)
        delete a
    }
    t |> run("mixed sources stop at the shortest") @(t : T?) {
        var a <- [for (i in range(5)); i + 1]
        t |> equal(each_with_range(a), 5)
        delete a
    }
    t |> run("nested loops over one array") @(t : T?) {
        var a <- [for (i in range(4)); i]
        t |> equal(each_nested(a), 36)
        delete a
    }
    t |> run("table each keeps the generic path") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@each_table_stays_generic))
        var tab <- {1 => 10, 2 => 20}
        t |> equal(each_table_stays_generic(tab), 33)
        delete tab
    }
}
