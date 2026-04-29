require dastest/testing_boost public

struct Foo {
    a : int
}

[test]
def test_temp_containers ( t : T? ) {
    t |> run("temporary array operations") <| @ ( t : T? ) {
        var a : array<Foo>
        var et : Foo := Foo( a=2)
        array_ops(t, a, Foo( a=1), et)
    }
    t |> run("temporary table operations") <| @ ( t : T? ) {
        var a : table<int; Foo>
        table_ops ( t, a, Foo( a=1), Foo( a=2))
    }
}

def array_ops ( t : T?; var a:array<Foo>; var e:Foo; var et:Foo# implicit ) {
    reserve(a,10)
    for (i in range(10)) {
        push(a, Foo( a=i))
    }
    t |> equal(10, length(a))
    clear(a)
    t |> equal(0, length(a))
    t |> success(empty(a))
    push(a, e)
    push(a, e, 0)
    t |> equal(1, a[0].a)
    t |> equal(1, a[1].a)
    t |> equal(2, length(a))
    t |> equal(1, back(a).a)
    static_if (FAIL_TO_COMPILE) {
        push(a, et)             // can't push temporary value
        push(a, et, 0)
    }
    var b := a
    t |> equal(1, b[0].a)
    t |> equal(1, b[1].a)
    t |> equal(2, length(b))
    clear(a)
    t |> equal(0, length(a))
    a := b
    t |> equal(2, length(a))
    emplace(a, e)
    t |> equal(3, length(a))
    t |> equal(0, e.a)
    if (FAIL_TO_COMPILE) {
        emplace(a, et)          // can't emplace temporary value
        emplace(a, et, 0)
    }
    pop(a)
    erase(a, 0)
    erase(a, 0, 1)
    t |> equal(0, length(a))
    a := b
    for (v in each(a)) {
        v.a = 2
    }
    for (v in each(a)) {
        t |> equal(2, v.a)
    }
    t |> equal(2, a[0].a)
    t |> equal(2, a[1].a)
    t |> equal(2, length(a) )
}


def table_ops ( t : T?; var a:table<int;Foo>; var e:Foo; var et:Foo implicit ) {
    for (i in range(10)) {
        a |> emplace(i,Foo( a=i))
    }
    t |> equal(10, length(a))
    clear(a)
    t |> equal(0, length(a))
    t |> success(empty(a))
    a |> insert(0, e)
    a |> insert(1, e)
    t |> equal(1, a |> get_value(0).a)
    t |> equal(1, a |> get_value(1).a)
    t |> equal(2, length(a) )
    clear(a)
    a |> insert(0, e)
    a |> insert(1, e)
    var b := a
    a := b
    t |> equal(1, b |> get_value(0).a)
    t |> equal(1, b |> get_value(1).a)
    t |> equal(2, length(b) )
    erase(a, 0)
    erase(a, 1)
    t |> equal(0, length(a))
    a := b
    t |> equal(1, a |> get_value(0).a)
    t |> equal(1, a |> get_value(1).a)
    t |> equal(2, length(a) )
    for (k in keys(a)) {
        t |> equal(1, a?[k]?.a ?? -1)
    }
    for (k in keys(a)) {
        t |> success ( k==0 || k==1 )
    }
    for (v in values(a)) {
        t |> equal(1, v.a)
    }
}
