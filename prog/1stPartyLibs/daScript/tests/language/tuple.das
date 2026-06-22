options gen2
require dastest/testing_boost public

// Tests tuple creation, named fields, positional access, sizeof, heap allocation,
// make_tuple syntax, fixed_array of tuples, and finalize/clone operations.

def testI(arg : tuple<i : int; f : float>) {
    return arg.i == 1
}

def testF(arg : tuple<i : int; f : float>) {
    return arg.f == 3.14
}

def overload_pick(x : tuple<int; float>) {
    return 1
}

def overload_pick(x : tuple<x : int; y : float>) {
    return 2
}

// return-position shorthand: positional bare-var tuple promotes to the
// function's declared named-tuple result type
def make_hit_return(eid : int; distSq : float) : tuple<eid : int; distSq : float> {
    return (eid, distSq)
}

def make_hit_locals() : tuple<eid : int; distSq : float> {
    let eid = 17
    let distSq = 4.25
    return (eid, distSq)
}

[test]
def test_tuple(t : T?) {
    t |> run("basic creation and named fields") @(t : T?) {
        let ttt = (i = 1, f = 3.14)
        t |> equal(testI(ttt), true)
        t |> equal(testF(ttt), true)
        t |> equal(ttt._first, 1)
    }
    t |> run("positional access and assignment") @(t : T?) {
        var tp : tuple<int; float; string>
        // sizeof is either 16 (64-bit) or 12 (32-bit)
        let sz = typeinfo sizeof(tp)
        t |> equal(sz == 16 || sz == 12, true)
        tp._0 = 1
        tp._1 = 2.0
        tp._2 = "3"
        var q = tp
        t |> equal(q._0, 1)
        t |> equal(q._1, 2.0)
        t |> equal(q._2, "3")
    }
    t |> run("pointer access") @(t : T?) {
        var tp : tuple<int; float; string>
        tp._0 = 1
        tp._1 = 2.0
        tp._2 = "3"
        unsafe {
            var pq = addr(tp)
            t |> equal(pq._0, 1)
            t |> equal(pq._1, 2.0)
            t |> equal(pq._2, "3")
            t |> equal(pq?._0 ?? 0, 1)
            t |> equal(pq?._1 ?? 0.0, 2.0)
            t |> equal(pq?._2 ?? "nothing", "3")
        }
    }
    t |> run("heap allocated tuple") @(t : T?) {
        var qq = new <tuple<int; float>>
        qq._0 = 1
        qq._1 = 2.0
        t |> equal(qq?._0 ?? 0, 1)
        t |> equal(qq?._1 ?? 0.0, 2.0)
        unsafe {
            delete qq
        }
    }
    t |> run("make tuple syntax") @(t : T?) {
        let mkt = (1, 2.0)
        t |> equal(mkt._0, 1)
        t |> equal(mkt._1, 2.0)
    }
    t |> run("shorthand named into array of named tuples") @(t : T?) {
        var arr : array<tuple<eid : int; distSq : float>>
        let eid = 7
        let distSq = 2.5
        arr |> push((eid, distSq))
        t |> equal(length(arr), 1)
        t |> equal(arr[0].eid, 7)
        t |> equal(arr[0].distSq, 2.5)
    }
    t |> run("shorthand named single element") @(t : T?) {
        let eid = 9
        var tup : tuple<eid : int> = (eid,)
        t |> equal(tup.eid, 9)
    }
    t |> run("shorthand named via typed var init") @(t : T?) {
        let eid = 11
        let distSq = 4.5
        var tup : tuple<eid : int; distSq : float> = (eid, distSq)
        t |> equal(tup.eid, 11)
        t |> equal(tup.distSq, 4.5)
    }
    t |> run("unnamed target keeps unnamed (no shorthand)") @(t : T?) {
        var arr : array<tuple<int; float>>
        let k = 11
        let v = 4.5
        arr |> push((k, v))   // backward compat: target is unnamed, no promotion
        t |> equal(arr[0]._0, 11)
        t |> equal(arr[0]._1, 4.5)
    }
    t |> run("overload resolution prefers unnamed tuple when both present") @(t : T?) {
        var x = 123
        var y = 321.
        // both `overload_pick(tuple<int;float>)` and `overload_pick(tuple<x:int;y:float>)` visible.
        // `(x, y)` is unnamed `tuple<int;float>` — exact match unnamed overload.
        // shorthand-to-named promotion is fallback-only (fires when no candidate matches),
        // so unnamed overload wins → returns 1.
        t |> equal(overload_pick((x, y)), 1)
    }
    t |> run("explicit named tuple literal picks named overload") @(t : T?) {
        var x = 123
        var y = 321.
        // `(x=x, y=y)` parser tag as named tuple `tuple<x:int;y:float>` directly.
        // Exact match named overload → returns 2.
        t |> equal(overload_pick((x = x, y = y)), 2)
    }
    t |> run("mixed expression tuple stays unnamed") @(t : T?) {
        var arr : array<tuple<int; int>>
        let a = 1
        arr |> push((a, a + 1))   // 2nd elem not bare ExprVar → never tagged
        t |> equal(arr[0]._0, 1)
        t |> equal(arr[0]._1, 2)
    }
    t |> run("shorthand named via return from named-tuple-typed fn") @(t : T?) {
        let r = make_hit_return(7, 2.5)
        t |> equal(r.eid, 7)
        t |> equal(r.distSq, 2.5)
    }
    t |> run("shorthand named via return using local bare vars") @(t : T?) {
        let r = make_hit_locals()
        t |> equal(r.eid, 17)
        t |> equal(r.distSq, 4.25)
    }
    t |> run("return-promoted tuple pushes into named-tuple array") @(t : T?) {
        var arr : array<tuple<eid : int; distSq : float>>
        arr |> push(make_hit_return(3, 1.5))
        t |> equal(arr[0].eid, 3)
        t |> equal(arr[0].distSq, 1.5)
    }
    t |> run("fixed array of tuples") @(t : T?) {
        let mkta = fixed_array<tuple<int; float>>((1, 2.0), (3, 4.0))
        t |> equal(mkta[0]._0, 1)
        t |> equal(mkta[0]._1, 2.0)
        t |> equal(mkta[1]._0, 3)
        t |> equal(mkta[1]._1, 4.0)
        let ti = fixed_array(1, 2, 3, 4)
        t |> equal(typeinfo sizeof(ti), 16)
        t |> equal(ti[0], 1)
        t |> equal(ti[3], 4)
        let tt = fixed_array((1, 2), (3, 4))
        t |> equal(typeinfo sizeof(tt), 16)
        t |> equal(tt[0]._0, 1)
        t |> equal(tt[1]._1, 4)
    }
    t |> run("finalize and clone") @(t : T?) {
        var x : tuple<int; float; array<int>>
        x._0 = 1
        x._1 = 2.0
        x._2 <- [for (xx in range(10)); xx]
        var y := x
        t |> equal(y._0, 1)
        t |> equal(y._1, 2.0)
        for (i in range(10)) {
            t |> equal(y._2[i], i)
        }
        delete x
        t |> equal(x._0, 0)
        t |> equal(x._1, 0.0)
        t |> equal(length(x._2), 0)
        // y should still be intact (it was cloned)
        t |> equal(y._0, 1)
        t |> equal(y._1, 2.0)
        for (i in range(10)) {
            t |> equal(y._2[i], i)
        }
    }
}
