options gen2
require dastest/testing_boost public

// Tests tuple creation, named fields, positional access, sizeof, heap allocation,
// make_tuple syntax, fixed_array of tuples, and finalize/clone operations.

def testI(arg : tuple<i : int; float>) {
    return arg.i == 1
}

def testF(arg : tuple<int; f : float>) {
    return arg.f == 3.14
}

[test]
def test_tuple(t : T?) {
    t |> run("basic creation and named fields") @(t : T?) {
        let ttt = (1, 3.14)
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
