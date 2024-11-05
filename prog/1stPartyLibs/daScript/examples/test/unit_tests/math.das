require math

options optimize=false

[sideeffects]
def rightPolicyCeili(a:float):int
    return ceili(a)

[export]
def test
    assert(uint32_hash(1U) != 1U)
    var seed:uint = 1U
    let position:int = 1
    assert(min(float2(0.5, -1.), float2(-1., 0.5)) == float2(-1.,-1.))
    assert(min(float4(0.5, -1.,0.5,-1.), float4(-1., 0.5,-1., 0.5)) == float4(-1,-1,-1,-1))
    assert(reflect(float3(1,1,0),float3(-1,0,0)) == float3(-1,1,0))
    assert(reflect(float2(1,1),float2(-1,0)) == float2(-1,1))
    assert(lerp(100., 0., 0.5) == 50.)
    assert(lerp(float2(100, 50), float2(), 0.5) == float2(50, 25))
    assert(lerp(float3(100, 50, 200), float3(), 0.5) == float3(50, 25, 100))
    assert(lerp(float4(100, 50, 200, 400), float4(), 0.5) == float4(50, 25, 100, 200))
    assert(lerp(float2(100, 50), float2(), float2(0.5)) == float2(50, 25))
    assert(lerp(float3(100, 50, 200), float3(), float3(0.5)) == float3(50, 25, 100))
    assert(lerp(float4(100, 50, 200, 400), float4(), float4(0.5)) == float4(50, 25, 100, 200))
    //assert(clamp(acos(2.), 0., 0.) == 0.)  // removed due to exception on Win32
    assert(clamp(100., 0., 200.) == 100.)
    assert(clamp(-100., 100., 200.) == 100.)
    assert(clamp(1000., 100., 200.) == 200.)
    assert(uint_noise_1D(position, seed) != seed)
    assert(uint_noise_2D(int2(100,100), seed) != seed)
    assert(uint_noise_3D(int3(100,100,100), seed) != seed)
    assert(dot(float2(0,1), float2(1,0))==0.)
    assert(dot(float3(0,1,0), float3(1,0,1))==0.)
    assert(dot(float4(0,1,0,0), float4(1,0,1,1))==0.)
    //
    assert(length(float2(0,0))==0.)
    assert(length(float3(0,0,0))==0.)
    assert(length(float4(0,0,0,0))==0.)
    assert(length_sq(float2(0,0))==0.)
    assert(length_sq(float3(0,0,0))==0.)
    assert(length_sq(float4(0,0,0,0))==0.)
    assert(length(float2(0,1))==1.)
    assert(length(float3(0,1,0))==1.)
    assert(length(float4(0,0,0,1))==1.)
    assert(length_sq(float2(0,1))==1.)
    assert(length_sq(float3(1,0,0))==1.)
    assert(length_sq(float4(0,1,0,0))==1.)
    assert(cross(float3(0,1,0), float3(0,0,1))==float3(1,0,0))
    //
    assert(sin(0.)==0.)
    assert(cos(0.)==1.)
    assert(tan(0.)==0.)
    assert(asin(0.)==0.)
    assert(acos(1.)==0.)
    assert(abs(asin(sin(0.5)) - 0.5) < 0.0001)
    assert(abs(acos(cos(0.5)) - 0.5) < 0.0001)
    assert(sin(0.1)*sin(0.1) + cos(0.1)*cos(0.1) == 1.)//cos^2 + sin^2 == 1
    //
    assert(atan2(0.,0.)==0.)
    assert(sqrt(4.)==2.)
    assert(sqrt(0.)==0.)
    assert(pow(2.,2.)==4.)
    assert(ceil(1.1)==2.)
    assert(floor(1.1)==1.)
    assert(ceil(-1.1)==-1.)
    assert(floor(-1.1)==-2.)
    assert(exp(0.)==1.)
    assert(abs(log(exp(1.)) - 1.) < 1e-6)
    assert(pow(0., 0.)==1.)
    assert(pow(2., 2.)==4.)
    assert(exp2(2.)==4.)
    assert(log2(4.)==2.)
    assert(abs(exp(1.) - 2.718281828459045f) < 1e-6)
    assert(abs(log(2.718281828459045f) - 1.) < 1e-6)
    assert(ceili(float2(0.5,-0.1)) == int2(1,0))
    assert(floori(float2(0.5,-0.1)) == int2(0,-1))
    assert(roundi(float2(0.501,-0.1)) == int2(1,0))
    assert(trunci(float2(0.501,-0.1)) == int2(0,0))
    // and doubles
    assert(ceili(0.5lf) == 1)
    assert(floori(0.5lf) == 0)
    assert(roundi(0.501lf) == 1)
    assert(trunci(0.501lf) == 0)
    // policy AOT
    verify(rightPolicyCeili(1.2)==2)
    // constants
    let mme = "{FLT_MIN} {FLT_MAX} {FLT_EPSILON}"
    assert(mme=="1.1754944e-38 3.4028235e+38 1.1920929e-07")
    return true