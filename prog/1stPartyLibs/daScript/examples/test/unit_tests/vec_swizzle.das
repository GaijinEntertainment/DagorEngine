options no_unused_function_arguments = false

def test_vec2(a:auto(vec2) -const -&)
    var f2_x  = vec2(1,2)
    let x : auto(numT) = f2_x[0]
    assert("{f2_x}"=="1,2")
    var f2_y = f2_x.yx
    assert("{f2_y}"=="2,1")
    f2_x.x = numT(3)
    f2_x.y = numT(4)
    assert("{f2_x}"=="3,4")
    f2_x.xy = vec2(5,6)
    assert("{f2_x}"=="5,6")
    f2_y = f2_x.xy
    assert("{f2_y}"=="5,6")

def test_vec3(a:auto(vec3) -const -&)
    var f3_x  = vec3(1,2,3)
    let x : auto(numT) = f3_x[0]
    let y : auto(vec2) = f3_x.xy
    assert("{f3_x}"=="1,2,3")
    var f3_y = f3_x.zyx
    assert("{f3_y}"=="3,2,1")
    f3_x.x = numT(3)
    f3_x.y = numT(4)
    f3_x.z = numT(5)
    assert("{f3_x}"=="3,4,5")
    f3_x.xyz = vec3(5,6,7)
    assert("{f3_x}"=="5,6,7")
    f3_y = f3_x.xyz
    assert("{f3_y}"=="5,6,7")
    f3_x.xy = vec2(8,9)
    assert("{f3_x}"=="8,9,7")
    f3_x.yz = vec2(1,2)
    assert("{f3_x}"=="8,1,2")
    assert(f3_x.zx==vec2(2,8))

def test_vec4(a:auto(vec4) -const -&)
    var f4_x  = vec4(1,2,3,4)
    let x : auto(numT) = f4_x[0]
    let y : auto(vec2) = f4_x.xy
    let z : auto(vec3) = f4_x.xyz
    assert("{f4_x}"=="1,2,3,4")
    var f4_y = f4_x.wzyx
    assert("{f4_y}"=="4,3,2,1")
    f4_x.x = numT(3)
    f4_x.y = numT(4)
    f4_x.z = numT(5)
    f4_x.w = numT(6)
    assert("{f4_x}"=="3,4,5,6")
    f4_x.xyzw = vec4(5,6,7,8)
    assert("{f4_x}"=="5,6,7,8")
    f4_y = f4_x.xyzw
    assert("{f4_y}"=="5,6,7,8")
    f4_x.xyz = vec3(8,9,0)
    assert("{f4_x}"=="8,9,0,8")
    f4_x.yz = vec2(1,2)
    assert("{f4_x}"=="8,1,2,8")
    assert(f4_x.zx==vec2(2,8))

[export]
def test
    test_vec2(float2())
    test_vec3(float3())
    test_vec4(float4())
    test_vec2(int2())
    test_vec3(int3())
    test_vec4(int4())
    test_vec2(uint2())
    test_vec3(uint3())
    test_vec4(uint4())
    return true

