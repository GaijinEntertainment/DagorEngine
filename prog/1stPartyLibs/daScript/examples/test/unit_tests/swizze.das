 // options log = true, log_nodes =true, print_ref = true, print_var_access = true // , log_optimization_passes = true

// options logAot=true, log_nodes=true

struct Foo
    t : bool
    v : int3 = int3(1,2,3)

def funV(v:int3):int3
    return v

[export]
def test
    // REGULAR V
    var v = int3(1,2,3)
    assert(v==int3(1,2,3))
    assert(v.x==1 && v.y==2 && v.z==3)
    assert(v.xy==int2(1,2) && v.yz==int2(2,3) && v.zx==int2(3,1))
    assert(v.yx==int2(2,1) && v.zy==int2(3,2) && v.xz==int2(1,3))
    assert(funV(v)==int3(1,2,3))
    assert(funV(v).x==1 && funV(v).y==2 && funV(v).z==3)
    v.x = 11
    assert(v==int3(11,2,3))
    v.y = 22
    assert(v==int3(11,22,3))
    v.z = 33
    assert(v==int3(11,22,33))
    assert(v.x==11 && v.y==22 && v.z==33)
    v.xy = int2(1,2)
    assert(v.x==1 && v.y==2 && v.z==33)
    v.yz = int2(2,3)
    assert(v.x==1 && v.y==2 && v.z==3)
    v.xyz = int3(11,22,33)
    assert(v.xyz==int3(11,22,33))
    // V in the structure
    var fv = Foo()
    assert(fv.v==int3(1,2,3))
    assert(fv.v.x==1 && fv.v.y==2 && fv.v.z==3)
    assert(fv.v.xy==int2(1,2) && fv.v.yz==int2(2,3) && fv.v.zx==int2(3,1))
    assert(fv.v.yx==int2(2,1) && fv.v.zy==int2(3,2) && fv.v.xz==int2(1,3))
    assert(funV(fv.v)==int3(1,2,3))
    assert(funV(fv.v).x==1 && funV(fv.v).y==2 && funV(fv.v).z==3)
    fv.v.x = 11
    assert(fv.v==int3(11,2,3))
    fv.v.y = 22
    assert(fv.v==int3(11,22,3))
    fv.v.z = 33
    assert(fv.v==int3(11,22,33))
    assert(fv.v.x==11 && fv.v.y==22 && fv.v.z==33)
    fv.v.xy = int2(1,2)
    assert(fv.v.x==1 && fv.v.y==2 && fv.v.z==33)
    fv.v.yz = int2(2,3)
    assert(fv.v.x==1 && fv.v.y==2 && fv.v.z==3)
    fv.v.xyz = int3(11,22,33)
    assert(fv.v.xyz==int3(11,22,33))
    return true
