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
// samplerCube (cubemap), sampler2DArray (layered 2D), sampler2DShadow (depth-compare 2D) are all
// core GLSL + GLES 3.0 / WebGL2 portable, so they live in the modern shared surface. Each carries the
// same GL-superset texture-unit handle field for the dasOpenGL rail (set host-side, ignored by SPIR-V).
struct samplerCube     { textureCube : uint }
struct sampler2DArray  { texture2DArray : uint }
struct sampler2DShadow { texture2D : uint }
// image3D is the volume storage image, addressed by an int3 (x, y, z). Both rails speak it: the GLSL
// emitter writes its layout/access qualifiers alongside image2D's, and the SPIR-V emitter lowers it
// to an OpTypeImage with Dim=3D. Arrayed storage images (image2DArray, imageCubeArray) stay SPIR-V-only.
struct image3D         { texture3D : uint }

// ===== vertex / fragment / point builtins =====
var gl_Position   : float4    // clip-space output position (vertex stage)
var gl_PointSize  : float     // rasterised point size, POINT topology
var gl_PointCoord : float2    // UV within a rasterised point, [0,1]^2 (fragment stage)
var gl_FragCoord  : float4    // window-space fragment coordinate, .xy in pixels (fragment stage); GLSL name-identical, SPIR-V BuiltIn FragCoord
var gl_FragDepth  : float     // fragment depth Output (fragment stage); GLSL name-identical (GL core + ES 3.0), SPIR-V BuiltIn FragDepth -- writing it forces the DepthReplacing execution mode
// gl_VertexIndex / gl_InstanceIndex are the unified (Vulkan) spellings -- the SPIR-V emitter lowers them to BuiltIn VertexIndex / InstanceIndex; the GLSL emitter renames them to gl_VertexID / gl_InstanceID (the GLSL desktop-core / ES names for the same per-vertex / per-instance index) on the way out. Author shaders with the one unified name on both rails.
var gl_VertexIndex   : int    // per-vertex index (vertex stage)
var gl_InstanceIndex : int    // per-instance index (vertex stage, instanced draws)

// ===== compute builtins =====
var gl_NumWorkGroups        : uint3   // global dispatch size (glDispatchCompute args)
var gl_WorkGroupID          : uint3   // this invocation's work group, in the global grid
var gl_LocalInvocationID    : uint3   // this invocation's position within its work group
var gl_GlobalInvocationID   : uint3   // unique invocation index in the global grid
var gl_LocalInvocationIndex : uint    // 1D flattening of gl_LocalInvocationID
// The declared workgroup size (the shader's own local_size_{x,y,z}) -- a COMPILE-TIME CONSTANT, not
// a dispatch input: the SPIR-V emitter folds it to a BuiltIn WorkgroupSize OpConstantComposite. A
// module shared by several shaders (each with its own local_size) cannot name that size any other way.
var gl_WorkGroupSize        : uint3   // the local_size the shader was compiled with

// The subgroup IDs, KHR_shader_subgroup spellings — both rails speak them (SPIR-V: the Subgroup*
// BuiltIns; MSL: the simdgroup thread-position attributes, noted per line). A CPU replay that
// sets gl_SubgroupSize = 1u and gl_SubgroupInvocationID = 0u keeps width-1 semantics exact.
var gl_SubgroupSize         : uint = 1u   // lanes per subgroup (Metal: [[threads_per_simdgroup]]); width-1 CPU default
var gl_SubgroupInvocationID : uint        // this lane's index within its subgroup ([[thread_index_in_simdgroup]])
var gl_NumSubgroups         : uint = 1u   // subgroups per workgroup ([[simdgroups_per_threadgroup]])
var gl_SubgroupID           : uint        // this subgroup's index within the workgroup ([[simdgroup_index_in_threadgroup]])

// ===== matrix constructors =====
// matCxR(col0, ...) builds a matrix from its columns (GLSL core `mat4(vec4,...)`); daslang matrices
// are column-major, so the emitter lowers the call to one OpCompositeConstruct (glslang's mat4(cols)).
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

// ===== row-vector times matrix =====
// GLSL's `v * m` (a ROW vector times m) that math lacks: component j is dot(v, column j), exactly
// SPIR-V's OpVectorTimesMatrix, which the emitter lowers to -- so this CPU body runs only on the host.
def public operator *(v : float4; m : float4x4) : float4 {
    return float4(dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3]))
}

def public operator *(v : float3; m : float3x3) : float3 {
    return float3(dot(v, m[0]), dot(v, m[1]), dot(v, m[2]))
}

// ===== vector integer modulo =====
// GLSL's `uvec3 % uvec3` that daslang lacks -> one native OpUMod (the emitter lowers it). Unsigned,
// 3-lane only; signed `%` stays absent since daslang's OpSRem and GLSL's OpSMod differ on sign.
def public operator %(a, b : uint3) : uint3 {
    return uint3(a.x % b.x, a.y % b.y, a.z % b.z)
}

// ===== float modulo =====
// GLSL's mod(x,y) = x - y*floor(x/y) (sign-of-DIVISOR, one OpFMod) vs daslang's fmod (sign-of-
// dividend, OpFRem): they differ on a negative dividend, so a mod() port must reach THIS, not fmod.
[unused_argument(x, y), sideeffects]
def public mod(x, y : float) : float { return 0.0 }

[unused_argument(x, y), sideeffects]
def public mod(x, y : float3) : float3 { return float3(0.0, 0.0, 0.0) }

// ===== 16-bit integer arithmetic =====
// daslang's 16-bit ints are storage + converts only; these close +/- and lower to native OpIAdd /
// OpISub / OpSNegate on OpTypeInt 16 (glslang's i16vec2). `*` `/` absent: GLSL widens `2*s` to int.
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

// ===== 16-bit ordering comparisons =====
// daslang rejects `<` `<=` `>` `>=` on storage types (only `==` `!=` are defined); GLSL compares a
// 16-bit pair natively (glslang gives one OpSGreaterThanEqual on two %short), which these restore.
def public operator <(a, b : int16) : bool { return int(a) < int(b) }
def public operator <=(a, b : int16) : bool { return int(a) <= int(b) }
def public operator >(a, b : int16) : bool { return int(a) > int(b) }
def public operator >=(a, b : int16) : bool { return int(a) >= int(b) }

// The unsigned twin: glslang picks the UNSIGNED opcode (OpUGreaterThanEqual) for uint16, so these
// cannot fold into the signed set above -- a uint16 pair compared through int() would take the signed
// op and order 0xffff BELOW 0.
def public operator <(a, b : uint16) : bool { return uint(a) < uint(b) }
def public operator <=(a, b : uint16) : bool { return uint(a) <= uint(b) }
def public operator >(a, b : uint16) : bool { return uint(a) > uint(b) }
def public operator >=(a, b : uint16) : bool { return uint(a) >= uint(b) }

// ===== 16-bit max =====
// daslang declares max only over its 32-bit numerics, so a 16-bit max would otherwise widen both
// operands and narrow the result -- three converts around glslang's one OpExtInst SMax on two %short.
def public max(a, b : int16) : int16 { return int16(max(int(a), int(b))) }

// ===== 16-bit vector constructors from 16-bit lanes =====
// daslang builds a shortN from INT lanes but not from 16-bit ones, so without these a 16-bit add
// re-enters a vector via two OpSConverts per lane; with them one bare OpCompositeConstruct, as glslang.
def public short2(x, y : int16) : short2 { return short2(int(x), int(y)) }
def public short3(x, y, z : int16) : short3 { return short3(int(x), int(y), int(z)) }
def public short3(xy : short2; z : int16) : short3 { return short3(int(xy.x), int(xy.y), int(z)) }
def public short4(x, y, z, w : int16) : short4 { return short4(int(x), int(y), int(z), int(w)) }
def public short4(xyz : short3; w : int16) : short4 {
    return short4(int(xyz.x), int(xyz.y), int(xyz.z), int(w))
}

// The unsigned twin, for a u16vec4 shuffled lane by lane. Only the 4-lane form has a glslang
// reference to gate it; the narrower ones would be an untested lowering, so they stay a compile
// error, exactly as the 16-bit `*` and `/` do.
def public ushort4(x, y, z, w : uint16) : ushort4 {
    return ushort4(uint(x), uint(y), uint(z), uint(w))
}

// ===== 16-bit float vector constructors from 16-bit lanes =====
// daslang builds a halfN from FLOAT lanes and narrows a whole vector, but cannot COMPOSE one from
// narrower half pieces; with these the constituents already ARE the component type -- one OpCompositeConstruct.
def public half3(xy : half2; z : float16) : half3 {
    return half3(float(xy.x), float(xy.y), float(z))
}

def public half4(xy, zw : half2) : half4 {
    return half4(float(xy.x), float(xy.y), float(zw.x), float(zw.y))
}

// GLSL converts a 16-bit float straight to int (OpConvertFToS is width-agnostic); daslang core
// declares no int(float16), so without this a port widens first and spends an OpFConvert the glslang
// reference does not. uint is absent -- no glslang reference gates it.
def public int(v : float16) : int { return int(float(v)) }

// ===== texture sampling =====
// texture(s, uv): filtered sample with implicit LOD (fragment-only on both rails). textureSize:
// the mip-0 dimensions in texels.
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

// textureLod samples with an EXPLICIT level-of-detail: no screen-space derivatives, so unlike
// texture() it is valid in any stage (vertex/compute), not fragment-only. GLSL name-identical
// (core + ES 3.0); SPIR-V lowers it to OpImageSampleExplicitLod with a Lod operand.
[unused_argument(s, uv, lod), sideeffects]
def public textureLod(s : sampler2D; uv : float2; lod : float) : float4 {
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

// texelFetch reads a single texel by integer coordinate + mip level, unfiltered. Stage-agnostic
// (no derivatives). GLSL name-identical (core + ES 3.0); SPIR-V lowers it to OpImage + OpImageFetch.
[unused_argument(s, coord, lod), sideeffects]
def public texelFetch(s : sampler2D; coord : int2; lod : int) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

// texture() on the wider sampler dimensionalities: a samplerCube samples by a float3 direction, a sampler2DArray by (u, v, layer). Both GLSL name-identical (the emitter's texture() is dimension- agnostic); SPIR-V keys the marker by name (Dim Cube / Arrayed). textureSize returns the mip-level size in texels (int2 for cube, int3 for 2D-array / 3D = width,height,layers-or-depth).
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

// texelFetch on a sampler2DArray indexes (x, y, layer) -- the layer is a plain array index, never normalized, so the coordinate is int3 while the sampler2D form takes int2. GLSL name-identical; the emitter's OpImage + OpImageFetch path is dimension-agnostic and needs no change.
[unused_argument(s, coord, lod), sideeffects]
def public texelFetch(s : sampler2DArray; coord : int3; lod : int) : float4 {
    return float4(0.0, 0.0, 0.0, 0.0)
}

// textureCompare samples a depth-comparison (shadow) sampler: returns the scalar compare result in [0,1] (1 = lit, 0 = shadowed; a filtered sampler returns the PCF average). Implicit LOD -> fragment-only. SPIR-V lowers it to OpImageSampleDrefImplicitLod; the GLSL emitter rewrites it to the GL spelling texture(sampler2DShadow, vec3(uv, compare)) (GL has no distinct shadow function).
[unused_argument(s, uv, compare), sideeffects]
def public textureCompare(s : sampler2DShadow; uv : float2; compare : float) : float {
    return 0.0
}

// ===== storage images =====
// image2D is a read-write storage image; imageStore writes a texel by integer coordinate,
// imageLoad reads one, imageSize queries the dimensions. Compute-capable rails only.
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

// image3D is addressed one coordinate wider than image2D; imageSize returns (width, height, depth).
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

// ===== compute synchronization =====
// barrier(): control + shared-memory barrier (every invocation in the work group rendezvous here).
// memoryBarrierShared(): order shared-memory writes without the execution rendezvous.
[sideeffects]
def public barrier() : void {}

[sideeffects]
def public memoryBarrierShared() : void {}

//! memoryBarrierBuffer(): order buffer (SSBO) writes so other invocations can read them — pair
//! as `memoryBarrierBuffer(); barrier();` for a cross-invocation buffer handoff (barrier() alone
//! only covers shared memory).
[sideeffects]
def public memoryBarrierBuffer() : void {}

// ===== fragment realism =====
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

// ===== fp16 packing =====
def public packHalf2x16(v : float2) : uint {
    return unsafe(reinterpret<uint>(half2(v)))
}

def public unpackHalf2x16(bits : uint) : float2 {
    return float2(unsafe(reinterpret<half2>(bits)))
}

// ===== 8-bit pack/unpack (GL_EXT_shader_explicit_arithmetic_types_int8 spellings) =====
// unpack8 splits one 32-bit scalar into four 8-bit lanes (input signedness picks byte4/ubyte4 —
// no return-type overloading, the GLSL extension's own trick); pack32 inverts. Real CPU bodies.
def public unpack8(x : int) : byte4 {
    return unsafe(reinterpret<byte4>(x))
}

def public unpack8(x : uint) : ubyte4 {
    return unsafe(reinterpret<ubyte4>(x))
}

def public pack32(v : byte4) : int {
    return unsafe(reinterpret<int>(v))
}

def public pack32(v : ubyte4) : uint {
    return unsafe(reinterpret<uint>(v))
}

// ===== packed 4x8-bit integer dot products (dp4a class) =====
// sdot4: 4+4 SIGNED int8 lanes; usdot4: a UNSIGNED x b SIGNED (quant shape, ARM usdot order).
// Real CPU bodies so CPU == GPU exactly; SPIR-V lowers to OpSDot/OpSUDot, MSL to as_type helpers.
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
