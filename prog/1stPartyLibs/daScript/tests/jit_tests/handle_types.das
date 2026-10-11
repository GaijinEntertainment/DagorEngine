options gen2
require dastest/testing_boost

require daslib/math_boost
require UnitTest

def handle_fun(var a : float4x4) {
    return inverse(a)
}

def handle_ref_op1(a : float4x4) {
    let c = -a
    return c
}

def handle_ref_op2(a, b : float4x4) {
    let c = a * b
    return c
}

def handle_val(var num : int&) {
    let iid = make_invalid_id()
    num = int(iid)
    let iid2 = EntityId(13)
    return iid2
}

def handle_eid(l : lambda<(val : int) : EntityId>) {
    return invoke(l, 13)
}

def handle_fields(var m : float4x4) {
    m.x = float4(1, 0, 0, 0)
    m.y = float4(0, 2, 0, 0)
    m.z = float4(0, 0, 3, 0)
    m.w = float4(0, 0, 0, 4)
    return m.y
}

def handle_fields_ptr(var m : float4x4?) {
    m.x = float4(1, 0, 0, 0)
    m.y = float4(0, 2, 0, 0)
    m.z = float4(0, 0, 3, 0)
    m.w = float4(0, 0, 0, 4)
    return m.y
}

def handle_index(var m : float4x4) {
    m[0] = float4(1, 0, 0, 0)
    m[1] = float4(0, 2, 0, 0)
    m[2] = float4(0, 0, 3, 0)
    m[3] = float4(0, 0, 0, 4)
    return m.y
}

def handle_vector_each(arr : Point3Array) {
    var n = 0
    var sum = 0.0
    for (p in arr) {
        n++
        sum += p.x + p.y + p.z
    }
    return int(sum) * 100 + n
}

def handle_vector_at(arr : Point3Array) {
    var sum = 0.0
    for (i in range(arr |> length)) {
        sum += arr[i].y * float(i + 1)
    }
    return int(sum)
}

[test]
def test_handle_type(t : T?) {
    t |> run("handle val") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_val))
        var num = 0
        let iid = handle_val(num)
        t |> equal(13, int(iid))
        t |> equal(-1, num)
    }
    t |> run("handle eid") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_eid))
        let l <- @(var val : int) : EntityId {
            return EntityId(val)
        }
        let eid = handle_eid(l)
        t |> equal(13, int(eid))
    }
    t |> run("handle op1") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_ref_op1))
        var a, b, c : float4x4
        identity(a)
        b = handle_ref_op1(a)
        identity(c)
        t |> equal(-c, b)
    }
    t |> run("handle op2") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_ref_op2))
        var a, b, c, d : float4x4
        identity(a)
        identity(b)
        c = handle_ref_op2(a, b)
        identity(d)
        t |> equal(d, c)
    }
    t |> run("handle fun") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_fun))
        var a : float4x4
        identity(a)
        a[1][1] = 2.0
        a[2][2] = 4.0
        a[3][0] = 1.0
        a[3][1] = 2.0
        a[3][2] = 4.0
        let b = handle_fun(a)
        let c = inverse(a)
        t |> equal(c, b)
    }
    t |> run("handle fields") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_fields))
        var m : float4x4
        let my = handle_fields(m)
        t |> equal(float4(1, 0, 0, 0), m.x)
        t |> equal(float4(0, 2, 0, 0), m.y)
        t |> equal(float4(0, 0, 3, 0), m.z)
        t |> equal(float4(0, 0, 0, 4), m.w)
        t |> equal(float4(0, 2, 0, 0), my)
    }
    t |> run("handle index") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_index))
        var m : float4x4
        let my = handle_index(m)
        t |> equal(float4(1, 0, 0, 0), m.x)
        t |> equal(float4(0, 2, 0, 0), m.y)
        t |> equal(float4(0, 0, 3, 0), m.z)
        t |> equal(float4(0, 0, 0, 4), m.w)
        t |> equal(float4(0, 2, 0, 0), my)
    }
    t |> run("handle fields ptr") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_fields_ptr))
        var m : float4x4
        let my = handle_fields_ptr(unsafe(addr(m)))
        t |> equal(float4(1, 0, 0, 0), m.x)
        t |> equal(float4(0, 2, 0, 0), m.y)
        t |> equal(float4(0, 0, 3, 0), m.z)
        t |> equal(float4(0, 0, 0, 4), m.w)
        t |> equal(float4(0, 2, 0, 0), my)
    }
}

[test]
def test_handle_vector(t : T?) {
    t |> run("handle vector each") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_vector_each))
        testPoint3Array() $(arr) {
            t |> equal(6010, handle_vector_each(arr))
        }
    }
    t |> run("handle vector at") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@handle_vector_at))
        testPoint3Array() $(arr) {
            t |> equal(110, handle_vector_at(arr))
        }
    }
}
