// Tests struct operations: creation, pointers, null-safe field chains (?.), ref args
options gen2
require dastest/testing_boost public

struct Dummy {}

struct Ball {
    position : float3
    radius : float
}

struct Balls {
    a, b : Ball
}

def get_ball_radius(b : Ball) : float {
    return b.radius
}

def get_balls_radius(b : Balls) : float {
    return b.a.radius + b.b.radius
}

def set_default_ball(var b : Ball) {
    b.position = float3(1.0, 2.0, 3.0)
    b.radius = 5.0
}

def make_default_ball : Ball ? {
    var b : Ball? = new Ball
    set_default_ball(deref(b))
    return b
}

def verify_default_ball(t : T?; b : Ball) {
    t |> equal(b.position, float3(1.0, 2.0, 3.0))
    t |> equal(b.radius, 5.0)
}

def test_ref_ptr(t : T?; var a : Ball?&; var b : Ball?) {
    a = b
    t |> equal(a, b)
}

def test_ret_ptr(var a : Ball?) : Ball? {
    return a
}

struct L3 {
    a : int
}

struct L2 {
    l3 : L3?
}

struct L1 {
    l2 : L2?
}

var glob : L1? = null

[test]
def test_struct(t : T?) {
    t |> run("empty struct sizeof") @(t : T?) {
        t |> equal(typeinfo sizeof(type<Dummy>), 0)
    }
    t |> run("struct pointer and copy") @(t : T?) {
        var b : Ball? = new Ball
        b.position = float3(1.0, 2.0, 3.0)
        b.radius = 5.0
        verify_default_ball(t, deref(b))
        var xb, qb : Ball
        var qc : Ball? = b
        var qd : Ball? = new Ball
        qb = deref(qc)
        verify_default_ball(t, qb)
        verify_default_ball(t, deref(qc))
        qc = null
        t |> equal(qc, null)
        t |> success(qd != null)
        test_ref_ptr(t, qc, qd)
        t |> equal(qc, qd)
        qd = test_ret_ptr(null)
        t |> equal(qd, null)
        t |> success(qd != qc)
        xb = qb
        verify_default_ball(t, xb)
    }
    t |> run("make_default_ball via ptr") @(t : T?) {
        let b : Ball? = make_default_ball()
        verify_default_ball(t, deref(b))
    }
    t |> run("null-safe field chain") @(t : T?) {
        var dummy : int
        glob?.l2?.l3?.a ?? dummy = 1
        t |> equal(dummy, 1)
        glob = new L1
        glob?.l2?.l3?.a ?? dummy = 2
        t |> equal(dummy, 2)
        glob.l2 = new L2
        glob?.l2?.l3?.a ?? dummy = 3
        t |> equal(dummy, 3)
        glob.l2.l3 = new L3
        glob?.l2?.l3?.a ?? dummy = 4
        t |> equal(dummy, 3)
        t |> equal(glob?.l2?.l3?.a ?? 5, 4)
    }
}
