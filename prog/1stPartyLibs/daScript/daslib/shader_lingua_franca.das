options gen2
options indenting = 4

// Shader lingua franca: the backend-invariant builtin surface shared by daslang's two shader
// emitter cores -- dasGlsl (-> GLSL text) and dasSpirv (-> SPIR-V binary). Both
// `require daslib/shader_lingua_franca public`, so every existing `require glsl/glsl_common` /
// `require spirv/spirv_builtins` consumer keeps seeing these symbols unchanged.
//
// All declarations here are AST-only: the bodies never execute in daslang. Each emitter reads the
// call / reference AST and lowers it (a GLSL builtin call, or a SPIR-V opcode / BuiltIn-decorated
// variable). Recognition is by SYMBOL name in spirv_emit, and by MODULE name (this module) in
// glsl_internal -- both emitters treat a symbol from here exactly as they treat their own builtins.
//
// Only the MODERN WebGL2 / GL-4.1-core surface that BOTH rails speak lives here. GL legacy
// (GLSL 1.x: gl_FragColor, texture2D()) stays in glsl_common; VK-specific intrinsics
// (samplerCube, subpassInput, atomics, mesh, ...) stay in spirv_builtins.

module shader_lingua_franca shared public

// float4x4 (the matrix constructors below) lives in math; the vector types are builtin.
require math

// ===== opaque resource types =====
// GL-superset: the `texture2D`/`texture3D` uint carries the GL texture-unit handle the host binds
// (glUniform1i). The SPIR-V emitter recognizes each type by NAME (classify_global / sampler_info)
// and ignores the field entirely -- it is a pure descriptor marker there. Shader code never reads
// the field; it only passes the value to texture()/imageLoad()/imageStore().
struct sampler2D       { texture2D : uint }
struct sampler3D       { texture3D : uint }
struct image2D         { texture2D : uint }
struct samplerCube     { textureCube : uint }
struct sampler2DArray  { texture2DArray : uint }
struct sampler2DShadow { texture2D : uint }
struct image3D         { texture3D : uint }

var gl_Position   : float4
var gl_PointSize  : float
var gl_PointCoord : float2
var gl_FragCoord  : float4
var gl_FragDepth  : float
var gl_VertexIndex   : int
var gl_InstanceIndex : int

var gl_NumWorkGroups        : uint3
var gl_WorkGroupID          : uint3
var gl_LocalInvocationID    : uint3
var gl_GlobalInvocationID   : uint3
var gl_LocalInvocationIndex : uint
var gl_WorkGroupSize        : uint3

var gl_SubgroupSize         : uint = 1u
var gl_SubgroupInvocationID : uint
var gl_NumSubgroups         : uint = 1u
var gl_SubgroupID           : uint

def public float4x4(c0, c1, c2, c3 : float4) : float4x4 {
    var m : float4x4
    m[0] = c0
    m[1] = c1
    m[2] = c2
    m[3] = c3
    return m
}

def public float3x3(c0, c1, c2 : float3) : float3x3 {
    var m : float3x3
    m[0] = c0
    m[1] = c1
    m[2] = c2
    return m
}

def public operator *(v : float4; m : float4x4) : float4 {
    return float4(dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3]))
}

def public operator *(v : float3; m : float3x3) : float3 {
    return float3(dot(v, m[0]), dot(v, m[1]), dot(v, m[2]))
}

def public operator %(a, b : uint3) : uint3 {
    return uint3(a.x % b.x, a.y % b.y, a.z % b.z)
}

[unused_argument(x, y), sideeffects]
def public mod(x, y : float) : float { return 0.0 }

[unused_argument(x, y), sideeffects]
def public mod(x, y : float3) : float3 { return float3(0.0, 0.0, 0.0) }

def public operator +(a, b : int16) : int16 { return int16(int(a) + int(b)) }
def public operator +(a, b : short2) : short2 { return short2(int2(a) + int2(b)) }
def public operator +(a, b : short3) : short3 { return short3(int3(a) + int3(b)) }
def public operator +(a, b : short4) : short4 { return short4(int4(a) + int4(b)) }

def public operator -(a, b : int16) : int16 { return int16(int(a) - int(b)) }
def public operator -(a, b : short2) : short2 { return short2(int2(a) - int2(b)) }
def public operator -(a, b : short3) : short3 { return short3(int3(a) - int3(b)) }
def public operator -(a, b : short4) : short4 { return short4(int4(a) - int4(b)) }

def public operator -(a : int16) : int16 { return int16(-int(a)) }
def public operator -(a : short2) : short2 { return short2(-int2(a)) }
def public operator -(a : short3) : short3 { return short3(-int3(a)) }
def public operator -(a : short4) : short4 { return short4(-int4(a)) }

def public operator <(a, b : int16) : bool { return int(a) < int(b) }
def public operator <=(a, b : int16) : bool { return int(a) <= int(b) }
def public operator >(a, b : int16) : bool { return int(a) > int(b) }
def public operator >=(a, b : int16) : bool { return int(a) >= int(b) }

def public operator <(a, b : uint16) : bool { return uint(a) < uint(b) }
def public operator <=(a, b : uint16) : bool { return uint(a) <= uint(b) }
def public operator >(a, b : uint16) : bool { return uint(a) > uint(b) }
def public operator >=(a, b : uint16) : bool { return uint(a) >= uint(b) }

def public max(a, b : int16) : int16 { return int16(max(int(a), int(b))) }

def public short2(x, y : int16) : short2 { return short2(int(x), int(y)) }
def public short3(x, y, z : int16) : short3 { return short3(int(x), int(y), int(z)) }
def public short3(xy : short2; z : int16) : short3 { return short3(int(xy.x), int(xy.y), int(z)) }
def public short4(x, y, z, w : int16) : short4 { return short4(int(x), int(y), int(z), int(w)) }
def public short4(xyz : short3; w : int16) : short4 {
    return short4(int(xyz.x), int(xyz.y), int(xyz.z), int(w))
}

def public ushort4(x, y, z, w : uint16) : ushort4 {
    return ushort4(uint(x), uint(y), uint(z), uint(w))
}

def public half3(xy : half2; z : float16) : half3 {
    return half3(float(xy.x), float(xy.y), float(z))
}

def public half4(xy, zw : half2) : half4 {
    return half4(float(xy.x), float(xy.y), float(zw.x), float(zw.y))
}

def public int(v : float16) : int { return int(float(v)) }

[unused_argument(s, uv), sideeffects]
def public texture(s : sampler2D; uv : float2) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv), sideeffects]
def public texture(s : sampler3D; uv : float3) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, lod), sideeffects]
def public textureSize(s : sampler2D; lod : int) : int2 {
    return int2(0, 0)
}

[unused_argument(s, uv, lod), sideeffects]
def public textureLod(s : sampler2D; uv : float2; lod : float) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv, lod), sideeffects]
def public textureLod(s : samplerCube; uv : float3; lod : float) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv, lod), sideeffects]
def public textureLod(s : sampler3D; uv : float3; lod : float) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv, lod), sideeffects]
def public textureLod(s : sampler2DArray; uv : float3; lod : float) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, coord, lod), sideeffects]
def public texelFetch(s : sampler2D; coord : int2; lod : int) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv), sideeffects]
def public texture(s : samplerCube; uv : float3) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv), sideeffects]
def public texture(s : sampler2DArray; uv : float3) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, lod), sideeffects]
def public textureSize(s : samplerCube; lod : int) : int2 {
    return int2(0, 0)
}

[unused_argument(s, lod), sideeffects]
def public textureSize(s : sampler3D; lod : int) : int3 {
    return int3(0, 0, 0)
}

[unused_argument(s, lod), sideeffects]
def public textureSize(s : sampler2DArray; lod : int) : int3 {
    return int3(0, 0, 0)
}

[unused_argument(s, coord, lod), sideeffects]
def public texelFetch(s : sampler2DArray; coord : int3; lod : int) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(s, uv, compare), sideeffects]
def public textureCompare(s : sampler2DShadow; uv : float2; compare : float) : float {
    return 0.0
}

[unused_argument(img, uv, data), sideeffects]
def public imageStore(img : image2D; uv : int2; data : float4) : void {}

[unused_argument(img, uv), sideeffects]
def public imageLoad(img : image2D; uv : int2) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(img), sideeffects]
def public imageSize(img : image2D) : int2 {
    return int2(0, 0)
}

[unused_argument(img, uvw, data), sideeffects]
def public imageStore(img : image3D; uvw : int3; data : float4) : void {}

[unused_argument(img, uvw), sideeffects]
def public imageLoad(img : image3D; uvw : int3) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

[unused_argument(img), sideeffects]
def public imageSize(img : image3D) : int3 {
    return int3(0, 0, 0)
}

[sideeffects]
def public barrier() : void {}

[sideeffects]
def public memoryBarrierShared() : void {}

//! memoryBarrierBuffer(): order buffer (SSBO) writes so other invocations can read them — pair
//! as `memoryBarrierBuffer(); barrier();` for a cross-invocation buffer handoff (barrier() alone
//! only covers shared memory).
[sideeffects]
def public memoryBarrierBuffer() : void {}

[sideeffects]
def public discard() : void {}

[unused_argument(p), sideeffects] def public dFdx(p : float) : float { return p }
[unused_argument(p), sideeffects] def public dFdx(p : float2) : float2 { return p }
[unused_argument(p), sideeffects] def public dFdx(p : float3) : float3 { return p }
[unused_argument(p), sideeffects] def public dFdx(p : float4) : float4 { return p }
[unused_argument(p), sideeffects] def public dFdy(p : float) : float { return p }
[unused_argument(p), sideeffects] def public dFdy(p : float2) : float2 { return p }
[unused_argument(p), sideeffects] def public dFdy(p : float3) : float3 { return p }
[unused_argument(p), sideeffects] def public dFdy(p : float4) : float4 { return p }
[unused_argument(p), sideeffects] def public fwidth(p : float) : float { return p }
[unused_argument(p), sideeffects] def public fwidth(p : float2) : float2 { return p }
[unused_argument(p), sideeffects] def public fwidth(p : float3) : float3 { return p }
[unused_argument(p), sideeffects] def public fwidth(p : float4) : float4 { return p }

def public packHalf2x16(v : float2) : uint {
    return unsafe(reinterpret<uint>(half2(v)))
}

def public unpackHalf2x16(bits : uint) : float2 {
    return float2(unsafe(reinterpret<half2>(bits)))
}

def public unpack8(x : int) : byte4 {
    return unsafe(reinterpret<byte4>(x))
}

def public unpack8(x : uint) : ubyte4 {
    return unsafe(reinterpret<ubyte4>(x))
}

[arch(at="ARCHITECTURE_SHADER.md#shader-lingua-franca")]
def public unpack8(x : int16) : byte2 {
    return unsafe(reinterpret<byte2>(x))
}

[arch(at="ARCHITECTURE_SHADER.md#shader-lingua-franca")]
def public unpack8(x : uint16) : ubyte2 {
    return unsafe(reinterpret<ubyte2>(x))
}

def public pack32(v : byte4) : int {
    return unsafe(reinterpret<int>(v))
}

def public pack32(v : ubyte4) : uint {
    return unsafe(reinterpret<uint>(v))
}

def public sdot4(a, b : uint) : int {
    let av = int4(unpack8(int(a)))
    let bv = int4(unpack8(int(b)))
    return av.x * bv.x + av.y * bv.y + av.z * bv.z + av.w * bv.w
}

def public usdot4(a, b : uint) : int {
    let av = int4(uint4(unpack8(a)))
    let bv = int4(unpack8(int(b)))
    return av.x * bv.x + av.y * bv.y + av.z * bv.z + av.w * bv.w
}
