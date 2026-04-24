// const ref restrictions
// comprehensive const-correctness checks: writing to const values,
// moving to/from const, copying incompatible types, const pointers,
// const table indexing, and cross-module type incompatibility
options gen2
require _helper_foo

expect 30102     // passThrough
expect 30106
expect 30303:1    // operator_not_found
expect 30304:2    // function_not_found
expect 30504:5    // cant_write_to_const
expect 30505:2    // cant_move_to_const
expect 30507:3    // cant_copy
expect 30508:1    // cant_move

def const_iter(var a : table<string; int>) {
    for (k, v in keys(a), values(a)) {
        k = "foo"       // this is actually ok, its a temp variable
        v = 1           // this one okay
    }
}

def add(a : int const&; b : int) {
    a += b              // 30504: numeric operator += left side can't be constant.
}

def set(a : string const&; b : string) {
    a = b               // 30504: can't write to a constant value, string const& = string const&
}

def move_ar(a : array<int> const; b : array<int>) {
    a <- b              // 30505: can't move to a constant value, array<int> const <- array<int> const
}

struct Foo {
    a : int
    b : int const
}

struct TestObjectFoo {
    a : int
}

def setFoo(var f : Foo) {
    f.a = 0
}

def setFooPtr(var f : Foo?) {
    f.a = 0
}

def goodPassThrough(var a : int?) : int const? {
    return a            // fine
}

def passThrough(a : int? const) : int? {
    return a            // error[30102]: incompatible return type, expecting int?, passing int? const
}

def test {
    {
        var a : array<int> const
        a[0] = 5        // 30504: can't write to a constant value, int const& = int const
    }
    {
        var an : Foo
        an.a = 1
        an.b = 2        // 30504: can't write to a constant value, int const& = int const
    }
    {
        var ac : Foo const
        ac.a = 3        // 30504: can't write to a constant value, int const& = int const
        ac.b = 4        // 30504: can't write to a constant value, int const& = int const
    }
    {
        var pa : Foo const?
        var pb : Foo?
        setFoo(deref(pa))    // 30304: no matching functions or generics setFoo ( Foo const& )
        setFoo(deref(pb))
        setFooPtr(pa)        // 30304: no matching functions or generics setFooPtr ( Foo const?& -const )
        setFooPtr(pb)
    }
    {
        var a : table<string; int>
        var b : table<string; int> const
        unsafe(a["key"])++
        unsafe(b["key"])++      // 30106: can't index in the constant table, use find instead
    }
    {
        var a : int?
        var b : int const?
        a = b
    }
    {
        var a, b : table<string; int>
        var c : table<string; int> const
        var d : int
        a = b           // 30303: can only copy compatible type, int?& -const = int const?& -const
                        // 30507: this type can't be copied, table<string;int>& -const = table<string;int>& -const
        a <- b
        c <- a          // 30505: can't move to a constant value, table<string;int> const& -const <- table<string;int>& -const
        c <- d          // 30508: can't move compatible type, table<string;int> const& -const <- int& -const
    }
    {
        var a, b : int
        a <- b
        a = b
    }
    {
        var a : ::TestObjectFoo
        var b : _helper_foo::TestObjectFoo
        a = b           // 30303: can only copy compatible type, TestObjectFoo& -const = _helper_foo::TestObjectFoo& -const
    }
}
