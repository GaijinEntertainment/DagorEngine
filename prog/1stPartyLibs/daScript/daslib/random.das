options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module random shared

require math

let LCG_RAND_MAX = 32767                                        //! maximum possible output of random number generator
let LCG_RAND_MAX_BIG = 32768*32768-1                            //! maximum possible output of random_big_int
let private LCG_IRAND_MAX_FLT = 1.0f / float(LCG_RAND_MAX)

[generic]
def random_seed ( seed:int )
    //! constructs seed vector out of single integer seed
    return int4(seed,seed+1,seed+2,seed+3)

[generic]
def random_seed2D(var seed:int4&; co:int2; cf:int=0)
    //! constructs seed vector out of 2d screen coordinates and frame counter `cf`
    let prnd = sin(dot(float2(co.xy),float2(12.9898,78.233))) * 43758.5453
    seed = random_seed(int(prnd)+cf)
    return random_float4(seed)

[generic]
def random_int ( var seed:int4& )
    //! random integer 0..32767 (LCG_RAND_MAX)
    seed.x = 214013*seed.x+2531011
    return (seed.x>>16) & LCG_RAND_MAX

[generic]
def random_big_int ( var seed:int4& )
    //! random integer 0..32768*32768-1 (LCG_RAND_MAX_BIG)
    seed = int4(214013)*seed+int4(2531011)
    let i12 = (seed.xy>>16) & int2(LCG_RAND_MAX)
    return i12.x + i12.y*32768

[generic]
def random_uint ( var seed:int4& )
    //! random integer 0..32768*32768-1 (LCG_RAND_MAX_BIG)
    seed = int4(214013)*seed+int4(2531011)
    let i123 = (seed.xyz>>16) & int3(LCG_RAND_MAX)
    return uint(i123.x) ^ (uint(i123.y)<<15u) ^ (uint(i123.z)<<30u)

[generic]
def random_int4 ( var seed:int4& )
    //! random int4, each component is 0..32767 (LCG_RAND_MAX)
    seed = int4(214013)*seed+int4(2531011)
    return (seed>>16) & int4(LCG_RAND_MAX)

[generic]
def random_float ( var seed:int4& )
    //! random float 0..1
    return float(random_int(seed)) * LCG_IRAND_MAX_FLT

[generic]
def random_float4 ( var seed:int4& )
    //! random float4, each component is 0..1
    return float4(random_int4(seed)) * float4(LCG_IRAND_MAX_FLT)

[generic]
def random_unit_vector ( var seed:int4& )
    //! random float3 unit vector (length=1.)
    let zz = random_float4(seed)
    let z = zz.x * 2. - 1.
    let a = zz.y * ( 2. * PI )
    let r = sqrt(1. - z*z)
    var x, y : float
    sincos(a, x, y)
    return float3( r * x, r * y, z )

[generic]
def random_in_unit_sphere ( var seed:int4& )
    //! random float3 unit vector (length=1) which happens to be inside a sphere R=1
    while true
        let r = random_float4(seed).xyz
        let p = float3(2.)*r - float3(1.)
        if length_sq(p) <= 1.0
            return p
    return float3(0.)

[generic]
def random_in_unit_disk ( var seed:int4& )
    //! random float3 unit vector (length=1) which happens to be inside a disk R=1, Z=0
    while true
        let R = random_float4(seed).xy
        let r = float3(R.x,R.y,0.)
        let p = float3(2.)*r - float3(1.,1.,0.)
        if length_sq(p) <= 1.0
            return p
    return float3(0.)

def each_random_uint(rnd_seed:int=13)
    return <- generator<uint>() <|
        var seed = random_seed(rnd_seed)
        while true
            yield random_uint(seed)
        return false
