options gen2
require dastest/testing_boost public

require daslib/random

options no_aliasing = false // these tests make sense only with disabled aliasing

var seed : int4

let FULL_TEST = false

struct Foo {
    a : int
    b : float4
}

struct Bar {
    f : Foo
}

// potential alias - cmres vs argument a
def set(a : Foo) {
    var b = Foo(a = 13)
    b.a += a.a
    return b
}

// potential alias - cmres.a vs argument inc
def set2(inc : int&) {
    var b = Foo(a = 13)
    b.a += inc
    return b
}

// no aliasing ever
def set2_ok(inc : int) {
    var b = Foo(a = 13)
    b.a += inc
    return b
}

// potential alias - cmres.b.{xyzw} vs argument incf
def set3(incf : float&) {
    var b = Foo(b = float4(13.))
    b.b += float4(incf)
    return b
}

// potential alias - cmres.a vs argument inc via *inc
def set4(inc : int?) {
    var b = Foo(a = 13)
    b.a += *inc
    return b
}

var g_a, g_b : Foo

var g_p_a = new Foo()
var g_p_b = g_p_a

// potential alias - global variable cmres vs global variable g_a
def set5 {
    var b = Foo(a = 13)
    b.a += g_a.a
    return b
}

def get_a : Foo {
    return g_a
}

// potential alias - global variable cmres vs global variable g_a via get_a function
def set6 {
    var b = Foo(a = 13)
    b.a += get_a().a
    return b
}

// this function ALWAYS aliases its result, becase it reads g_p_a pointer
def set7 {
    var b = Foo(a = 13)
    b.a += g_p_a?.a ?? 100500
    return b
}

def pass_ref_a(var a : Foo) : Foo& {
    unsafe {
        return a
    }
}

def set_with_extra(var b : Foo; extra : int) : Foo {
    var a = Foo(a = extra)
    a.a += b.a
    return a
}

[export]
def set_with_extra_which_touches_global_but_no_addr_taken(var b : Foo; extra : int) : Foo {
    b.a += g_a.a + extra
    return b
}

def set_with_2_extra(var b : Foo; e1, e2 : int) : Foo {
    var a = Foo(a = e1 + e2)
    a.a += b.a
    return a
}

[export]
def set_with_2_extra_which_touches_global(var b : Foo; e1, e2 : int) : Foo {
    b.a += g_a.a + e1 + e2
    return b
}

[test]
def test_aliasing(t : T?) {
    t |> run("straight up aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b : Foo
        a = set(a)                  // (1) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = set(a)                  // this one ok
        t |> equal(14, b.a)
    }
    t |> run("field aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b : Foo
        a = set2(a.a)               // (2) 40211: function set result aliases argument inc
        t |> equal(14, a.a)
        a = Foo(a = 1)
        a = set2_ok(a.a)            // this one ok
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = set2(a.a)               // this one ok
        t |> equal(14, b.a)
    }
    t |> run("swizzle + vector subtype aliasing") @(t : T?) {
        var a = Foo(b = float4(1.))
        var b : Foo
        a = set3(a.b.x)             // (3) 40211: function set result aliases argument incf
        t |> equal(14., a.b.x)
        a = Foo(b = float4(1.))
        b = set3(a.b.y)             // this one ok
        t |> equal(14., b.b.y)
    }
    t |> run("array [] aliasing") @(t : T?) {
        var a, b : Foo[2]
        a[1] = Foo(a = 1)
        a[0] = set(a[1])            // (4) 40211: function set result aliases argument a
                                        // note - false aliasing
        t |> equal(14, a[0].a)
        a[1] = Foo(a = 1)
        b[0] = set(a[1])            // this one ok
        t |> equal(14, b[0].a)
    }
    t |> run("array ?[] aliasing, ?? aliasing") @(t : T?) {
        var a, b : Foo[2]
        var defv = Foo(a = 2)
        a[0] = Foo(a = 1)
        a?[0] ?? defv = set(a[0])   // (5) 40211: function set result aliases argument a
        t |> equal(14, a[0].a)
        t |> equal(2, defv.a)
        a[0] = Foo(a = 1)
        a?[1] ?? defv = set(defv)   // (6) 40211: function set result aliases argument a
        t |> equal(15, a[1].a)
        t |> equal(2, defv.a)
        a[0] = Foo(a = 1)
        b?[0] ?? defv = set(a[0])   // this one ok
        t |> equal(14, b[0].a)
        t |> equal(2, defv.a)
        a[0] = Foo(a = 1)
        b?[3] ?? defv = set(a[0])   // this one ok
        t |> equal(14, defv.a)
    }
    t |> run("cast aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b = Foo(a = 2)
        a = set2(unsafe(reinterpret<int&> a.b)) // (7) 40211: function set result aliases argument inc
        t |> equal(13, a.a)
        a = Foo(a = 1)
        a = set2(unsafe(reinterpret<int&> b.b)) // this one ok
        t |> equal(13, a.a)
    }
    t |> run("op3 aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b = Foo(a = 2)
        var cond = (random_int(seed) & 1) != 0
        cond ? a : b = set(a)       // (8) 40211: function set result aliases argument a
        t |> equal(14, cond ? a.a : b.a)
        a = Foo(a = 1)
        b = Foo(a = 2)
        a = set(cond ? a : b)       // (9) 40211: function set result aliases argument a
        t |> equal(cond ? 14 : 15, a.a)
        a = Foo(a = 1)
        b = Foo(a = 2)
        a = set(cond ? b : b)       // this one ok
        t |> equal(15, a.a)
    }
    t |> run("ref2ptr aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b = Foo(a = 2)
        a = set4(unsafe(addr(a.a))) // (10) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = Foo(a = 2)
        b = set4(unsafe(addr(a.a))) // this one ok
        t |> equal(14, b.a)
        a = Foo(a = 1)
        b = Foo(a = 2)
        a = set4(unsafe(addr(b.a))) // this one ok
        t |> equal(15, a.a)
    }
    t |> run("ptr2ref, ref2value aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var pa = unsafe(addr(a))
        var paa = unsafe(addr(a.a))
        var b : Foo
        a = set2(*paa)      // (11) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        *pa = set2(a.a)     // (12) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = set2(*paa)      // (13) 40211: function set result aliases argument a
                                // note - false aliasing
        t |> equal(14, b.a)
        a = Foo(a = 1)
        set2(*paa)          // this oke ok
        t |> equal(1, a.a)
    }
    t |> run("global variable aliasing") @(t : T?) {
        g_a = Foo(a = 1)
        g_a = set5()        // (14) 40211: function set5 result aliases global variable ::g_a S<::Foo> through function set5
        t |> equal(14, g_a.a)
        g_a = Foo(a = 1)
        g_b = set5()        // this one ok
        t |> equal(14, g_b.a)
    }
    t |> run("global variable through function aliasing") @(t : T?) {
        g_a = Foo(a = 1)
        g_a = set6()        // (15) 40211: function set6 result aliases global variable ::g_a S<::Foo> through function get_a
        t |> equal(14, g_a.a)
        g_a = Foo(a = 1)
        g_b = set6()        // this one ok
        t |> equal(14, g_b.a)
    }
    t |> run("pointer argument false aliasing") @(t : T?) {
        var a = new Foo(a = 1)
        var b : Foo
        b = set(*a)         // (16) 40211: function set result aliases argument a
                                // note - false aliasing
        t |> equal(14, b.a)
    }
    t |> run("pointer result false aliasing") @(t : T?) {
        var a = Foo(a = 1)
        var b = new Foo()
        *b = set(a)     // (17) 40211: function set result always aliases
                                // note - false aliasing
        t |> equal(14, b.a)
    }
    t |> run("global variable pointer aliasing") @(t : T?) {
        *g_p_a = Foo(a = 1)   // (I) 40212: [[ExprMakeStruct ]] always aliases
        g_b = set7()    // (18) 40211: function set7 result aliases global variable ::g_p_a 1<S<::Foo>>? through function set7
                                // note - false aliasing
        t |> equal(14, g_b.a)
    }
    t |> run("pass_ref call aliasing") @(t : T?) {
        var a = Foo(a = 1)
        a = set(pass_ref_a(a))  // (19) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        a = set(move_to_local(a))      // this one ok
        t |> equal(14, a.a)
        a = Foo(a = 1)
        a = pass_ref_a(set(a))      // this one ok
        t |> equal(14, a.a)
    }
    t |> run("make struct aliasing") @(t : T?) {
        var b : Bar
        b.f = Foo(a = 1)
        b = Bar(f = set(b.f))    // (II) 40212: [[ExprMakeStruct ]] aliases
        t |> equal(14, b.f.a)
    }
    t |> run("make struct aliasing") @(t : T?) {
        var b2 : Bar[2]
        b2[0].f = Foo(a = 1)
        b2 = fixed_array(Bar(                      // (III) 40212: [[ExprMakeStruct ]] aliases
            f = Foo(a = 13)), Bar(
            f = Foo(a = b2[0].f.a + 13)
        ))
        t |> equal(13, b2[0].f.a)
        t |> equal(14, b2[1].f.a)
    }
    t |> run("straight up invoke function aliasing") @(t : T?) {
        var fn = @@set
        var a = Foo(a = 1)
        var b : Foo
        a = invoke(fn, a)                 // (20) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = invoke(fn, a)                 // this one ok
        t |> equal(14, b.a)
    }
    t |> run("global function without aliasing") @(t : T?) {
        var fn = @@set_with_extra
        g_a = Foo(a = 1)
        g_a = invoke(fn, Foo(a = 1), 13)              // this one ok
        t |> equal(14, g_a.a)
    }
    t |> run("global function with aliasing") @(t : T?) {
        var fn = @@set_with_2_extra
        fn = @@set_with_2_extra_which_touches_global     // we take address of function (anywhere in the program)
        fn = @@set_with_2_extra
        g_a = Foo(a = 1)
        g_a = invoke(fn, Foo(a = 1), 6, 7)             // (21) 40211: invoke result potentially aliases global g_a through function set_with_2_extra_which_touches_global S<::Foo> Ci Ci
        t |> equal(14, g_a.a)
    }
    t |> run("straight up lambda aliasing") @(t : T?) {
        var fn <- @(a : Foo) {
            var b = Foo(a = 13)
            b.a += a.a
            return b
        }
        var a = Foo(a = 1)
        var b : Foo
        a = invoke(fn, a)                 // (22) 40211: function set result aliases argument a
        t |> equal(14, a.a)
        a = Foo(a = 1)
        b = invoke(fn, a)                 // this one ok
        t |> equal(14, b.a)
    }
    t |> run("global lambda without aliasing") @(t : T?) {
        var fn <- @(var b : Foo; extra : int) : Foo {
            var a = Foo(a = extra)
            a.a += b.a
            return a
        }
        g_a = Foo(a = 1)
        g_a = invoke(fn, Foo(a = 1), 13)              // this one ok
        t |> equal(14, g_a.a)
    }
    t |> run("global lambda with aliasing") @(t : T?) {
        var fn <- @(var b : Foo; e1, e2 : int) : Foo {
            b.a += g_a.a + e1 + e2
            return b
        }
        fn <- @(var b : Foo; e1, e2 : int) : Foo {
            var a = Foo(a = e1 + e2)
            a.a += b.a
            return a
        }
        g_a = Foo(a = 1)
        g_a = invoke(fn, Foo(a = 1), 6, 7)             // (23) 40211: invoke result potentially aliases global g_a through lambda at d:\Work\daScript\tests\language\aliasing.das:303:12
        t |> equal(14, g_a.a)
    }
}
