
include "assert.sh"

hlsl {
 #if _HARDWARE_METAL
   #define BGRA_SWIZZLE(a) a.zyxw
   #define BGR_SWIZZLE(a) a.zyx
 #endif

 #ifndef BGRA_SWIZZLE
   #define BGRA_SWIZZLE(a) a
 #endif
 #ifndef BGR_SWIZZLE
   #define BGR_SWIZZLE(a) a
 #endif
}

hlsl {
#define GPU_TARGET 1
#define PI 3.14159265f
#define PIh 3.14159265h
##if (hardware.fsh_5_0 || hardware.ps4 || hardware.ps5)
  #define HAS_REVERSEBITS 1
##endif
#define SEPARATE_SAMPLER_OBJECT 1
#define double doubles_are_not_supported_by_some_gpus
#define double2 doubles_are_not_supported_by_some_gpus
#define double3 doubles_are_not_supported_by_some_gpus
#define double4 doubles_are_not_supported_by_some_gpus
}

macro DECL_POSTFX_TC_VS_SCR()
  hlsl {
    #define HALF_RT_TC_YSCALE -0.5
    #define RT_SCALE_HALF float2(0.5,-0.5)
  }
endmacro

macro DECL_POSTFX_TC_VS_RT()
  DECL_POSTFX_TC_VS_SCR()
endmacro

macro DISABLE_VFACE()
  hlsl(ps)
  {
    #undef INPUT_VFACE
    #undef MUL_VFACE
    #undef MUL_VFACE_SATURATE
    #undef SET_IF_IS_BACK_FACE
    #define INPUT_VFACE
    #define MUL_VFACE(a) (a)
    #define MUL_VFACE_SATURATE(a) saturate(a)
    #define SET_IF_IS_BACK_FACE(a, b)
  }
endmacro

hlsl(vs) {
  #define fixed half
  #define fixed2 half2
  #define fixed3 half3
  #define fixed4 half4

  #define fxsampler sampler
  #define fxsampler2D sampler2D

  #define hsampler sampler
  #define hsampler2D sampler2D

  #define fltsampler sampler
  #define fltsampler2D sampler2D
}

hlsl(cs) {
  #define fixed half
  #define fixed2 half2
  #define fixed3 half3
  #define fixed4 half4
}
//add if needed: FMT_SNORM16_ABGR FMT_SINT16_ABGR

macro PS4_DEF_TARGET_FMT_32_R()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_32_R)
    ##endif
  }
endmacro

macro PS4_DEF_TARGET_FMT_32_AR()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_32_AR)
    ##endif
  }
endmacro

macro PS4_DEF_TARGET_FMT_32_GR()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_32_GR)
    ##endif
  }
endmacro

macro PS4_DEF_TARGET_FMT_32_ABGR()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_32_ABGR)
    ##endif
  }
endmacro

macro PS4_DEF_TARGET_FMT_UNORM16_ABGR()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_UNORM16_ABGR)
    ##endif
  }
endmacro

macro PS4_DEF_TARGET_FMT_UINT16_ABGR()
  hlsl(ps)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma PSSL_target_output_format(default FMT_UINT16_ABGR)
    ##endif
  }
endmacro

//Enable vertex and instance offsets in VS for PS4
//NOTE: without (used) input buffers, no fetch shader is generated
//Vertex shader has no code to add offsets to vertex/instance IDs (done in fetch shader)
//so need to directly use S_INSTANCE_OFFSET_ID semantics and manually add offsets
//To add, use  xId =  __v_sad_u32(xOffsetId, 0, xId);
//Note: PS4 adds vertex offset to vertexId, DX11 is only uses vertex offset internally for vbuffer fetch
macro USE_INDIRECT_DRAW()
  hlsl(vs)
  {
    ##if hardware.ps4 || hardware.ps5
    #pragma argument(indirect-draw)
    ##endif
  }
endmacro

macro USE_PS5_WAVE32_MODE()
  if (hardware.ps5)
  {
    hlsl {
      #pragma argument(wavemode = wave32)
      #undef WaveReadLaneAt
      #define WaveReadLaneAt CrossLane32Read
    }
  }
endmacro

hlsl {
##if !hardware.ps4 && !hardware.ps5
  float max3(float a, float b, float c)
  {
    return max(a, max(b, c));
  }
  float min3(float a, float b, float c)
  {
    return min(a, min(b, c));
  }
  #define INVARIANT(x) x
##else
  #define INVARIANT(x) __invariant(x)
##endif
  float max3(float3 a) {return max3(a.x, a.y, a.z);}

  float max4(float a, float b, float c, float d)
  {
    return max(max(a, d), max(b, c));
  }
  float min4(float a, float b, float c, float d)
  {
    return min(min(a, d), min(b, c));
  }
  ##if hardware.dx11 || hardware.dx12 || hardware.metal
    #define PRECISE precise
  ##elif hardware.vulkan
    #if SHADER_COMPILER_DXC
      #define PRECISE precise
    #else
      #define PRECISE
    #endif
  ##else
    #define PRECISE
  ##endif
}
hlsl {
##if hardware.ps4 || hardware.ps5
  #define shadow2D(a, uv) a.SampleCmpLOD0(a##_cmpSampler, (uv).xy, (uv).z)
  #define shadow2DArray(a, uv) a.SampleCmpLOD0(a##_cmpSampler, (uv).xyz, (uv).w)
##else
  #define shadow2D(a, uv) a.SampleCmpLevelZero(a##_cmpSampler, (uv).xy, (uv).z)
  #define shadow2DArray(a, uv) a.SampleCmpLevelZero(a##_cmpSampler, (uv).xyz, (uv).w)
##endif
}

hlsl(vs) {
  #define SQRT_SAT(x)  sqrt(saturate(x))
##if hardware.ps4 || hardware.ps5
  #define tex2Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xyz, (uv).w)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLOD(a##_samplerstate, tc, lod, ofs)
  #define tex2DLodBindless(a, uv) (a).tex.SampleLOD((a).smp, (uv).xy, (uv).w)
  int4 D3DCOLORtoUBYTE4(float4 x) { return int4(x.zyxw * 255.001953); }
  #define VS_OUT_POSITION(name)      float4 name:S_POSITION;
  #define VSOUT_POSITION             S_POSITION
  #define CLIPPLANE_OUT
  #define CLIPPLANE(v)
  #define SV_InstanceID              S_INSTANCE_ID
  #define SV_VertexID                S_VERTEX_ID
  #define SV_PrimitiveID             S_PRIMITIVE_ID
  #define SV_instanceID              //catch common errors at compile time
  #define SV_vertexID
  #define SV_primitiveID
  #define SV_RenderTargetArrayIndex  S_RENDER_TARGET_INDEX
##elif hardware.dx11 || hardware.vulkan || hardware.metal
  #define tex2Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, (uv).w)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLevel(a##_samplerstate, tc, lod, ofs)
  ##if hardware.metal
  float4 tex2DLodBindless(TextureSampler a, float4 uv)
  {
    Texture2D tex = a.tex;
    SamplerState smp = a.smp;
    return tex.SampleLevel(smp, uv.xy, uv.w);
  }
  ##else
  #define tex2DLodBindless(a, uv) (a).tex.SampleLevel((a).smp, (uv).xy, (uv).w)
  ##endif
  #define VS_OUT_POSITION(name)      float4 name:SV_POSITION;
  #define VSOUT_POSITION             SV_POSITION
  #define CLIPPLANE_OUT              , out float oClip0 : SV_ClipDistance0
  #define CLIPPLANE(v)               oClip0 = v;
##else
  #define VS_OUT_POSITION(name)      float4 name:POSITION;
  #define VSOUT_POSITION             POSITION
  #define CLIPPLANE_OUT              
  #define CLIPPLANE(v)               
##endif

##if hardware.vulkan || hardware.metal
  //vulkan compiler uses internal definitions for HW_VERTEX_ID, based on internal compiler used
  #if SHADER_COMPILER_DXC
    #define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct) input_struct.vertexId -= input_struct.baseVertexId;
  #else
    #define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct)
  #endif
##else
  #define HW_VERTEX_ID uint vertexId: SV_VertexID;
  #define HW_BASE_VERTEX_ID error! not supported on this compiler/API
  #define HW_BASE_VERTEX_ID_OPTIONAL
  #define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct)
##endif

  #define HW_input_used_instance_id ,uint instance_id
  #define HW_used_instance_id ,instance_id
  #define HW_USED_INSTANCE_ID ,instance_id

// also adds multidraw related shader input attributes if available
//
// instance_offset_id: is the firstInstance parameter to a direct drawing command or the
//                     startInstanceLocation member of a structure consumed by an
//                     indirect drawing command. (Vulkan/PS4/PS5)
##if hardware.ps4 || hardware.ps5
  #define HW_USE_INSTANCE_ID  ,uint instance_id:SV_InstanceID,uint instance_offset_id:S_INSTANCE_OFFSET_ID
##elif hardware.vulkan
  #if SHADER_COMPILER_DXC
    #define HW_USE_INSTANCE_ID  , uint instance_id:SV_InstanceID \
                                , [[vk::builtin("BaseInstance")]] uint instance_offset_id:S_VK_BASE_INSTANCE
  #else
    #define HW_USE_INSTANCE_ID  ,uint instance_id:SV_InstanceID
  #endif
##else
  #define HW_USE_INSTANCE_ID  ,uint instance_id:SV_InstanceID
##endif
}

hlsl(ps) {
  #define SQRT_SAT(x)  sqrt(saturate(x))

##if hardware.ps4 || hardware.ps5
  #define VS_OUT_POSITION(name) float4 name:S_POSITION;
##else
  #define VS_OUT_POSITION(name) float4 name:SV_POSITION;
##endif

##if hardware.vulkan
  #define result_float1 float4
  #define result_float2 float4
  #define result_float3 float4
  #define make_result_float1(v) float4(v, v, v, v)
  #define make_result_float2(v) float4(v, v)
  #define make_result_float3(v) float4(v, v.x)
##else
  #define result_float1 float1
  #define result_float2 float2
  #define result_float3 float3
  #define make_result_float1(v) float1(v)
  #define make_result_float2(v) float2(v)
  #define make_result_float3(v) float3(v)
##endif

##if hardware.ps4 || hardware.ps5
  float3 ps4_mul_vface(uint face, float3 normal)
  {
    //http://www.humus.name/Articles/Persson_LowlevelShaderOptimization.pdf
    return asfloat(BitFieldInsert(face, asuint(normal), asuint(-normal)));
  }
  #define INPUT_VFACE , uint vface : SV_IsFrontFace
  #define MUL_VFACE(a) ps4_mul_vface(vface, a)
  #define MUL_VFACE_SATURATE(a) saturate((vface ? (a) : (-a)))
  #define IF_IS_BACK_FACE if (!vface)
  #define SET_IF_IS_BACK_FACE(name, val) if (!vface) name = val;
  #define tex2Dgrad(a, uv, dx, dy) a.SampleGradient(a##_samplerstate, (uv).xy, dx, dy)
  #define tex2Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xy, (uv).w)
  #define tex2DlodSampler(a, b, uv) a.SampleLOD(b##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xyz, (uv).w)
  #define tex3Dgrad(a, uv, dx, dy) a.SampleGradient(a##_samplerstate, uv, dx, dy)
  #define texCUBElod(a, uv) a.SampleLOD(a##_samplerstate, uv.xyz, uv.w)
  #define texCUBEArraylod(a, uv, lod) a.SampleLOD(a##_samplerstate, (uv).xyzw, lod)
  #define tex2Dproj(a, uv) a.Sample(a##_samplerstate, (uv).xy/(uv).w)
  #define tex2D(a, uv) a.Sample(a##_samplerstate, uv)
  #define tex3D(a, uv) a.Sample(a##_samplerstate, uv)
  #define texCUBE(a, uv) a.Sample(a##_samplerstate, uv)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureOffset(a, tc, ofs) a.Sample(a##_samplerstate, tc, ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLOD(a##_samplerstate, tc, lod, ofs)
  #define tex2DBindless(a, uv) (a).tex.Sample((a).smp, uv)
  #define tex2DBindlessSampler(a, b, uv) (a).tex.Sample((b).smp, uv)
  #define tex2DLodBindless(a, uv) (a).tex.SampleLOD((a).smp, (uv).xy, (uv).w)
  #define tex2DgradBindless(a, uv, dx, dy) (a).tex.SampleGradient((a).smp, (uv).xy, dx, dy)
  #define tex2DprojBindless(a, uv) tex2DBindless((a), (uv).xy/(uv).w)
  #define texCUBEBindless(a, uv) (a).tex.Sample((a).smp, uv)
  #define texCUBELodBindless(a, uv) (a).tex.SampleLOD((a).smp, uv.xyz, uv.w)
  #define SV_IsFrontFace             S_FRONT_FACE
  #define SV_Depth                   S_DEPTH_OUTPUT
  #define SV_DepthGreaterEqual       S_DEPTH_GE_OUTPUT
  #define SV_DepthLessEqual          S_DEPTH_LE_OUTPUT
  #define SV_Target                  S_TARGET_OUTPUT
  #define SV_Target0                 S_TARGET_OUTPUT0
  #define SV_Target1                 S_TARGET_OUTPUT1
  #define SV_Target2                 S_TARGET_OUTPUT2
  #define SV_Target3                 S_TARGET_OUTPUT3
  #define SV_Target4                 S_TARGET_OUTPUT4
  #define SV_Target5                 S_TARGET_OUTPUT5
  #define SV_Target6                 S_TARGET_OUTPUT6
  #define SV_Target7                 S_TARGET_OUTPUT7
  #define SV_SampleIndex             S_SAMPLE_INDEX
  #define half float
  #define half1 float1
  #define half2 float2
  #define half3 float3
  #define half4 float4
  #define VPOS                       S_POSITION
  #define POSITION                   S_POSITION

##elif hardware.dx11 || hardware.vulkan || hardware.metal

  #define VPOS                       SV_POSITION

  #define tex2Dgrad(a, uv, dx, dy) a.SampleGrad(a##_samplerstate, (uv).xy, dx, dy)
  #define tex2Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xy, (uv).w)
  #define tex2DlodSampler(a, b, uv) a.SampleLevel(b##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, (uv).w)
  #define tex3Dgrad(a, uv, dx, dy) a.SampleGrad(a##_samplerstate, uv, dx, dy)
  #define texCUBElod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, uv.w)
  #define texCUBEArraylod(a, uv, lod) a.SampleLevel(a##_samplerstate, (uv).xyzw, lod)
  #define tex2Dproj(a, uv) a.Sample(a##_samplerstate, (uv).xy/(uv).w)
  #define tex2D(a, uv) a.Sample(a##_samplerstate, uv)
  #define tex3D(a, uv) a.Sample(a##_samplerstate, uv)
  #define texCUBE(a, uv) a.Sample(a##_samplerstate, uv)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureOffset(a, tc, ofs) a.Sample(a##_samplerstate, tc, ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLevel(a##_samplerstate, tc, lod, ofs)

  #define INPUT_VFACE , bool vface : SV_IsFrontFace
  ##if hardware.vulkan
    // It seems that GLSL translator cant vectorize ?: and generates 3 if to gl_FrontFacing and
    // sets components individually each component in a separate branch.
    // We trick it to vectorize.
    #define MUL_VFACE(a) lerp((-a), (a), saturate(vface))
    #define MUL_VFACE_SATURATE(a) saturate(MUL_VFACE(a))
  ##else
    #define MUL_VFACE(a) ((vface) ? (a) : (-a))
    #define MUL_VFACE_SATURATE(a) saturate(((vface) ? (a) : (-a)))
  ##endif

  ##if hardware.metal
    #define IF_IS_BACK_FACE if (!vface)
    #define SET_IF_IS_BACK_FACE(name, val) if (!vface) name = val;
    float4 tex2DBindless(TextureSampler a, float2 uv)
    {
        Texture2D tex = a.tex;
        SamplerState smp = a.smp;
        return tex.Sample(smp, uv);
    }

    float4 tex2DBindlessSampler(TextureSampler a, TextureSampler b, float2 uv)
    {
        Texture2D tex = a.tex;
        SamplerState smp = b.smp;
        return tex.Sample(smp, uv);
    }

    float4 tex2DLodBindless(TextureSampler a, float4 uv)
    {
        Texture2D tex = a.tex;
        SamplerState smp = a.smp;
        return tex.SampleLevel(smp, uv.xy, uv.w);
    }

    float4 texCUBEBindless(TextureSamplerCube a, float3 uv)
    {
      TextureCube tex = a.tex;
      SamplerState smp = a.smp;
      return tex.Sample(smp, uv);
    }

    float4 texCUBELodBindless(TextureSamplerCube a, float4 uv)
    {
      TextureCube tex = a.tex;
      SamplerState smp = a.smp;
      return tex.SampleLevel(smp, uv.xyz, uv.w);
    }

    float4 tex2DprojBindless(TextureSampler a, float4 uv)
    {
      return tex2DBindless(a, uv.xy/uv.w);
    }

  ##else
    ##if hardware.vulkan
      #define IF_IS_BACK_FACE if (vface == 0)
      #define SET_IF_IS_BACK_FACE(name, val) if (vface == 0) name = val;
    ##else
      #define IF_IS_BACK_FACE if (!vface)
      #define SET_IF_IS_BACK_FACE(name, val) if (!vface) name = val;

      #define half float
      #define half1 float1
      #define half2 float2
      #define half3 float3
      #define half4 float4
    ##endif

    #define tex2DBindless(a, uv) (a).tex.Sample((a).smp, uv)
    #define tex2DBindlessSampler(a, b, uv) (a).tex.Sample((b).smp, uv)
    #define tex2DLodBindless(a, uv) (a).tex.SampleLevel((a).smp, (uv).xy, (uv).w)
    #define tex2DgradBindless(a, uv, dx, dy) (a).tex.SampleGradient((a).smp, (uv).xy, dx, dy)
    #define tex2DprojBindless(a, uv) tex2DBindless((a), (uv).xy/(uv).w)
    #define texCUBEBindless(a, uv) (a).tex.Sample((a).smp, uv)
    #define texCUBELodBindless(a, uv) (a).tex.SampleLevel((a).smp, uv.xyz, uv.w)

  ##endif
##else

  #define INPUT_VFACE , float vface : VFACE
  #define MUL_VFACE(a) ((a)*vface)
  #define MUL_VFACE_SATURATE(a) saturate((a)*vface)
  #define IF_IS_BACK_FACE if (vface < 0)
  #define SET_IF_IS_BACK_FACE(name, val) if (vface < 0) name = val;

#define SV_Depth                   DEPTH
#define SV_DepthGreaterEqual       DEPTH
#define SV_DepthLessEqual          DEPTH
#define SV_Target                  COLOR
#define SV_Target0                 COLOR0
#define SV_Target1                 COLOR1
#define SV_Target2                 COLOR2
#define SV_Target3                 COLOR3
#define SV_Target4                 COLOR4
#define SV_Target5                 COLOR5
#define SV_Target6                 COLOR6
#define SV_Target7                 COLOR7
#define tex2Dgrad(a, uv, dx, dy) tex2D(a, uv, dx, dy)

##endif
}
hlsl(cs) {
  #define SQRT_SAT(x)  sqrt(saturate(x))
##if hardware.ps4 || hardware.ps5
  #define tex2Dgrad(a, uv, dx, dy) a.SampleGradient(a##_samplerstate, (uv).xy, dx, dy)
  #define tex2Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLOD(a##_samplerstate, (uv).xyz, (uv).w)
  #define texCUBElod(a, uv) a.SampleLOD(a##_samplerstate, uv.xyz, uv.w)
  #define texCUBEArraylod(a, uv, lod) a.SampleLOD(a##_samplerstate, (uv).xyzw, lod)
  #define tex2Dproj(a, uv) a.Sample(a##_samplerstate, (uv).xy/(uv).w)
  #define tex2D(a, uv) a.Sample(a##_samplerstate, uv)
  #define tex3D(a, uv) a.Sample(a##_samplerstate, uv)
  #define texCUBE(a, uv) a.Sample(a##_samplerstate, uv)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureOffset(a, tc, ofs) a.Sample(a##_samplerstate, tc, ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLOD(a##_samplerstate, tc, lod, ofs)
  #define half float
  #define half1 float1
  #define half2 float2
  #define half3 float3
  #define half4 float4
  #define VPOS                       S_POSITION
  #define POSITION                   S_POSITION

##elif hardware.dx11 || hardware.vulkan || hardware.metal
  #define tex2Dgrad(a, uv, dx, dy) a.SampleGrad(a##_samplerstate, (uv).xy, dx, dy)
  #define tex2Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xy, (uv).w)
  #define tex3Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, (uv).w)
  #define texCUBElod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, uv.w)
  #define texCUBEArraylod(a, uv, lod) a.SampleLevel(a##_samplerstate, (uv).xyzw, lod)
  #define tex2Dproj(a, uv) a.Sample(a##_samplerstate, (uv).xy/(uv).w)
  #define tex2D(a, uv) a.Sample(a##_samplerstate, uv)
  #define tex3D(a, uv) a.Sample(a##_samplerstate, uv)
  #define texCUBE(a, uv) a.Sample(a##_samplerstate, uv)
  #define textureGather(a, tc) a.Gather(a##_samplerstate, tc)
  #define texelFetchOffset(a, tc, lod, ofs) a.Load(int3(tc, lod), ofs)
  #define textureOffset(a, tc, ofs) a.Sample(a##_samplerstate, tc, ofs)
  #define textureLodOffset(a, tc, lod, ofs) a.SampleLevel(a##_samplerstate, tc, lod, ofs)

##endif
}


hlsl(ps) {
  #define CLIP(a) clip(a)
  //#define CLIP(a) if ((a)<0) discard; else {};
  #define fixed half
  #define fixed2 half2
  #define fixed3 half3
  #define fixed4 half4

  #define fxsampler sampler
  #define fxsampler1D sampler1D
  #define fxsampler2D sampler2D
  #define fxsampler3D sampler3D
  #define fxsamplerCUBE samplerCUBE
  #define fxsampler2DShadow sampler2DShadow

  #define hsampler sampler
  #define hsampler1D sampler1D
  #define hsampler2D sampler2D
  #define hsampler3D sampler3D
  #define hsamplerCUBE samplerCUBE
  #define hsampler2DShadow sampler2DShadow

  #define fltsampler sampler
  #define fltsampler1D sampler1D
  #define fltsampler2D sampler2D
  #define fltsampler3D sampler3D
  #define fltsamplerCUBE samplerCUBE
  #define fltsampler2DShadow sampler2DShadow

  #define fx1tex2Dproj(tex_,tc_) ((fixed )tex2Dproj(tex_, tc_).r)
  #define fx2tex2Dproj(tex_,tc_) ((fixed2)tex2Dproj(tex_, tc_).rg)
  #define fx3tex2Dproj(tex_,tc_) ((fixed3)tex2Dproj(tex_, tc_).rgb)
  #define fx4tex2Dproj(tex_,tc_) ((fixed4)tex2Dproj(tex_, tc_))

  #define fx1tex2D(tex_,tc_) ((fixed )tex2D(tex_, tc_).r)
  #define fx2tex2D(tex_,tc_) ((fixed2)tex2D(tex_, tc_).rg)
  #define fx3tex2D(tex_,tc_) ((fixed3)tex2D(tex_, tc_).rgb)
  #define fx4tex2D(tex_,tc_) ((fixed4)tex2D(tex_, tc_))

  #define fx1tex3D(tex_,tc_) ((fixed )tex3D(tex_, tc_).r)
  #define fx2tex3D(tex_,tc_) ((fixed2)tex3D(tex_, tc_).rg)
  #define fx3tex3D(tex_,tc_) ((fixed3)tex3D(tex_, tc_).rgb)
  #define fx4tex3D(tex_,tc_) ((fixed4)tex3D(tex_, tc_))

  #define fx1texCUBE(tex_,tc_) ((fixed )texCUBE(tex_, tc_).r)
  #define fx2texCUBE(tex_,tc_) ((fixed2)texCUBE(tex_, tc_).rg)
  #define fx3texCUBE(tex_,tc_) ((fixed3)texCUBE(tex_, tc_).rgb)
  #define fx4texCUBE(tex_,tc_) ((fixed4)texCUBE(tex_, tc_))

  //#define half float
  //#define half2 float2
  //#define half3 float3
  //#define half4 float4

  #define h1tex2Dproj(tex_,tc_) ((half )tex2Dproj(tex_, tc_).r)
  #define h2tex2Dproj(tex_,tc_) ((half2)tex2Dproj(tex_, tc_).rg)
  #define h3tex2Dproj(tex_,tc_) ((half3)tex2Dproj(tex_, tc_).rgb)
  #define h4tex2Dproj(tex_,tc_) ((half4)tex2Dproj(tex_, tc_))

  #define h1tex2D(tex_,tc_) ((half )tex2D(tex_, tc_).r)
  #define h2tex2D(tex_,tc_) ((half2)tex2D(tex_, tc_).rg)
  #define h3tex2D(tex_,tc_) ((half3)tex2D(tex_, tc_).rgb)
  #define h4tex2D(tex_,tc_) ((half4)tex2D(tex_, tc_))

  #define h1tex3D(tex_,tc_) ((half )tex3D(tex_, tc_).r)
  #define h2tex3D(tex_,tc_) ((half2)tex3D(tex_, tc_).rg)
  #define h3tex3D(tex_,tc_) ((half3)tex3D(tex_, tc_).rgb)
  #define h4tex3D(tex_,tc_) ((half4)tex3D(tex_, tc_))

  #define h1texCUBE(tex_,tc_) ((half )texCUBE(tex_, tc_).r)
  #define h2texCUBE(tex_,tc_) ((half2)texCUBE(tex_, tc_).rg)
  #define h3texCUBE(tex_,tc_) ((half3)texCUBE(tex_, tc_).rgb)
  #define h4texCUBE(tex_,tc_) ((half4)texCUBE(tex_, tc_))

  #define h4tex2Dlod(tex_,tc_) ((half4)tex2Dlod(tex_, tc_))
}
hlsl {
##if hardware.metal
  #define CLAMP_BORDER(a, name, val) if (a.x <= 0.0f || a.x >= 1.0f || a.y <= 0.0f || a.y >= 1.0f) { name = val; }
##else
  #define CLAMP_BORDER(a, name, val)
##endif
  // target-specific macros
  #define LOOP [loop]
  #define UNROLL [unroll]
  #define BRANCH [branch]
  #define FLATTEN [flatten]

  //#define EMPTY_STRUCT(name) struct name {float4 unused:VPOS;}
  //#define DECLARE_UNUSED_MEMBER float4 unused:VPOS;
  #define EMPTY_STRUCT(name) struct name {}
  #define DECLARE_UNUSED_MEMBER
  #define RETURN_EMPTY_STRUCT(name)
  #define INIT_EMPTY_STRUCT(name)

  ##if hardware.ps4 || hardware.ps5
   #define reversebits(bits) ReverseBits( bits )
  ##endif
}

hlsl {
// unfortunatly DXC does not allow alias variables of the same semantic, screen pos
// in pixel shaders are used frequently and need one extra abstraction to work properly
// for all targets.
  #define HW_USE_SCREEN_POS
  #define GET_SCREEN_POS(vs_pos) vs_pos

#ifndef TEXELFETCH_DEFINED
#define TEXELFETCH_DEFINED 1

##if DEBUG
  #define DEBUG_ENABLE_BOUNDS_CHECKS 1
  static uint bounds_check_dim;
  static uint bounds_check_stride;
  static uint2 bounds_check_2dim;
  static uint3 bounds_check_3dim;

  void checkTexture2DBounds(int2 tc, uint2 dim, int lod, int file, int ln, int name)
  {
    ##assert(all(uint2(tc) < dim.xy), "[%s:%.f] Out of bounds: Texture '%s' has size (%.f, %.f) for lod %.f, but access to (%.f, %.f)",
                                      file, ln, name, dim.x, dim.y, lod, tc.x, tc.y);
  }
  void checkTexture2DArrayBounds(int3 tc, uint3 dim, int lod, int file, int ln, int name)
  {
    ##assert(all(uint3(tc) < dim.xyz), "[%s:%.f] Out of bounds: Texture '%s' has size (%.f, %.f, %.f) for lod %.f, but access to (%.f, %.f, %.f)",
                                       file, ln, name, dim.x, dim.y, dim.z, lod, tc.x, tc.y, tc.z);
  }
  void checkTexture3DBounds(int3 tc, uint3 dim, int lod, int file, int ln, int name)
  {
    ##assert(all(uint3(tc) < dim.xyz), "[%s:%.f] Out of bounds: 3D texture '%s' has size (%.f, %.f, %.f) for lod %.f, but access to (%.f, %.f, %.f)",
                                       file, ln, name, dim.x, dim.y, dim.z, lod, tc.x, tc.y, tc.z);
  }
  void checkBufferBounds(int tc, uint dim, int file, int ln, int name)
  {
    ##assert(uint(tc) < dim, "[%s:%.f] Out of bounds: Buffer '%s' has size (%.f), but access to (%.f)",
                              file, ln, name, dim, tc);
  }
  void checkStencilBounds(int2 tc, uint2 dim, int file, int ln, int name)
  {
    ##assert(all(uint2(tc) < dim), "[%s:%.f] Out of bounds: Texture `%s` has size (%.f, %.f), but access to (%.f, %.f)",
                                   file, ln, name, dim.x, dim.y, tc.x, tc.y);
  }
  #define CHECK_TEXTURE2D                             \
    uint3 dim;                                        \
    a.GetDimensions(lod, dim.x, dim.y, dim.z);        \
    checkTexture2DBounds(tc, dim.xy, lod, file, ln, name)
  #define CHECK_TEXTURE2D_EXPR(a, tc)                          \
    a.GetDimensions(bounds_check_2dim.x, bounds_check_2dim.y), \
    checkTexture2DBounds(tc, bounds_check_2dim.xy, 0, _FILE_, __LINE__, -1)
  #define CHECK_TEXTURE2DARRAY                        \
    uint4 dim;                                        \
    a.GetDimensions(lod, dim.x, dim.y, dim.z, dim.w); \
    checkTexture2DArrayBounds(tc, dim.xyz, lod, file, ln, name)
  #define CHECK_TEXTURE3D                             \
    uint4 dim;                                        \
    a.GetDimensions(lod, dim.x, dim.y, dim.z, dim.w); \
    checkTexture3DBounds(tc, dim.xyz, lod, file, ln, name)
  #define CHECK_TEXTURE3D_EXPR(a, tc)                          \
    a.GetDimensions(bounds_check_3dim.x, bounds_check_3dim.y, bounds_check_3dim.z), \
    checkTexture3DBounds(tc, bounds_check_3dim.xyz, 0, _FILE_, __LINE__, -1)
  #define CHECK_BUFFER(file, ln, name)                \
    uint dim;                                         \
    a.GetDimensions(dim);                             \
    checkBufferBounds(tc, dim, file, ln, name)
  #define CHECK_BUFFER_EXPR(a, tc)                    \
    a.GetDimensions(bounds_check_dim),                \
    checkBufferBounds(tc, bounds_check_dim, _FILE_, __LINE__, -1)
  #define CHECK_STRUCTURED_BUFFER(file, ln, name)     \
    uint dim, stride;                                 \
    a.GetDimensions(dim, stride);                     \
    checkBufferBounds(tc, dim, file, ln, name)
  #define CHECK_STRUCTURED_BUFFER_EXPR(a, tc)               \
    a.GetDimensions(bounds_check_dim, bounds_check_stride), \
    checkBufferBounds(tc, bounds_check_dim, _FILE_, __LINE__, -1)
  #define CHECK_STENCIL                               \
    uint2 dim;                                        \
    a.GetDimensions(dim.x, dim.y);                    \
    checkStencilBounds(tc, dim, file, ln, name)
##else
  #define CHECK_TEXTURE2D
  #define CHECK_TEXTURE2D_EXPR(a, tc) 0
  #define CHECK_TEXTURE2DARRAY
  #define CHECK_TEXTURE3D
  #define CHECK_TEXTURE3D_EXPR(a, tc) 0
  #define CHECK_BUFFER(file, ln, name)
  #define CHECK_BUFFER_EXPR(a, tc) 0
  #define CHECK_STRUCTURED_BUFFER(file, ln, name)
  #define CHECK_STRUCTURED_BUFFER_EXPR(a, tc) 0
  #define CHECK_STENCIL
##endif

  float4 texelFetchBase(Texture2D<float4> a, int2 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2D; return a.Load(int3(tc, lod)); }
  float3 texelFetchBase(Texture2D<float3> a, int2 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2D; return a.Load(int3(tc, lod)); }
  float2 texelFetchBase(Texture2D<float2> a, int2 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2D; return a.Load(int3(tc, lod)); }
  float  texelFetchBase(Texture2D<float>  a, int2 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2D; return a.Load(int3(tc, lod)); }
  float4 texelFetchBase(Texture2DArray<float4> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2DARRAY; return a.Load(int4(tc, lod)); }
  float3 texelFetchBase(Texture2DArray<float3> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2DARRAY; return a.Load(int4(tc, lod)); }
  float2 texelFetchBase(Texture2DArray<float2> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2DARRAY; return a.Load(int4(tc, lod)); }
  float  texelFetchBase(Texture2DArray<float>  a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE2DARRAY; return a.Load(int4(tc, lod)); }
  float4 texelFetchBase(Texture3D<float4> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE3D; return a.Load(int4(tc, lod)); }
  float3 texelFetchBase(Texture3D<float3> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE3D; return a.Load(int4(tc, lod)); }
  float2 texelFetchBase(Texture3D<float2> a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE3D; return a.Load(int4(tc, lod)); }
  float  texelFetchBase(Texture3D<float>  a, int3 tc, int lod, int file, int ln, int name) { CHECK_TEXTURE3D; return a.Load(int4(tc, lod)); }
  #define texelFetch(a, tc, lod) texelFetchBase(a, tc, lod, _FILE_, __LINE__, -1)

##if hardware.metal
  #define loadBufferBase(a, tc, file, ln, name) a.Load(tc)
  #define loadBuffer2Base(a, tc, file, ln, name) a.Load2(tc)
  #define loadBuffer3Base(a, tc, file, ln, name) a.Load3(tc)
  #define loadBuffer4Base(a, tc, file, ln, name) a.Load4(tc)

  #define loadBuffer(a, tc) a.Load(tc)
  #define loadBuffer2(a, tc) a.Load2(tc)
  #define loadBuffer3(a, tc) a.Load3(tc)
  #define loadBuffer4(a, tc) a.Load4(tc)

  #define storeBufferBase(a, tc, value, file, ln, name) a.Store(tc, value);
  #define storeBuffer2Base(a, tc, value, file, ln, name) a.Store2(tc, value);
  #define storeBuffer3Base(a, tc, value, file, ln, name) a.Store3(tc, value);
  #define storeBuffer4Base(a, tc, value, file, ln, name) a.Store4(tc, value);

  #define storeBuffer(a, tc, value) { CHECK_BUFFER(_FILE_, __LINE__, -1); a.Store(tc, value); }
  #define storeBuffer2(a, tc, value) { CHECK_BUFFER(_FILE_, __LINE__, get_name_##a); a.Store2(tc, value); }
  #define storeBuffer3(a, tc, value) { CHECK_BUFFER(_FILE_, __LINE__, -1); a.Store3(tc, value); }
  #define storeBuffer4(a, tc, value) { CHECK_BUFFER(_FILE_, __LINE__, -1); a.Store4(tc, value); }
##else
  float4 loadBufferBase(Buffer<float4> a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a[tc]; }
  float3 loadBufferBase(Buffer<float3> a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a[tc]; }
  float2 loadBufferBase(Buffer<float2> a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a[tc]; }
  float  loadBufferBase(Buffer<float>  a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a[tc]; }
  uint   loadBufferBase(Buffer<uint>   a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a[tc]; }
  // Used for overloading. Please prefer `structuredBufferAt`.
  uint   loadBufferBase(StructuredBuffer<uint>   a, int tc, int file, int ln, int name) { CHECK_STRUCTURED_BUFFER(file, ln, name); return a[tc];}
  uint   loadBufferBase(RWStructuredBuffer<uint> a, int tc, int file, int ln, int name) { CHECK_STRUCTURED_BUFFER(file, ln, name); return a[tc];}

  uint   loadBufferBase(ByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load(tc); }
  uint2  loadBuffer2Base(ByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load2(tc); }
  uint3  loadBuffer3Base(ByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load3(tc); }
  uint4  loadBuffer4Base(ByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load4(tc); }
  uint   loadBufferBase(RWByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load(tc); }
  uint2  loadBuffer2Base(RWByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load2(tc); }
  uint3  loadBuffer3Base(RWByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load3(tc); }
  uint4  loadBuffer4Base(RWByteAddressBuffer a, int tc, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); return a.Load4(tc); }

  #define loadBuffer(a, tc) loadBufferBase(a, tc, _FILE_, __LINE__, -1)
  #define loadBuffer2(a, tc) loadBuffer2Base(a, tc, _FILE_, __LINE__, -1)
  #define loadBuffer3(a, tc) loadBuffer3Base(a, tc, _FILE_, __LINE__, get_name_##a)
  #define loadBuffer4(a, tc) loadBuffer4Base(a, tc, _FILE_, __LINE__, -1)

  void storeBufferBase(RWByteAddressBuffer a, int tc, uint value, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); a.Store(tc, value); }
  void storeBuffer2Base(RWByteAddressBuffer a, int tc, uint2 value, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); a.Store2(tc, value); }
  void storeBuffer3Base(RWByteAddressBuffer a, int tc, uint3 value, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); a.Store3(tc, value); }
  void storeBuffer4Base(RWByteAddressBuffer a, int tc, uint4 value, int file, int ln, int name) { CHECK_BUFFER(file, ln, name); a.Store4(tc, value); }

  #define storeBuffer(a, tc, value) storeBufferBase(a, tc, value, _FILE_, __LINE__, -1)
  #define storeBuffer2(a, tc, value) storeBuffer2Base(a, tc, value, _FILE_, __LINE__, get_name_##a)
  #define storeBuffer3(a, tc, value) storeBuffer3Base(a, tc, value, _FILE_, __LINE__, -1)
  #define storeBuffer4(a, tc, value) storeBuffer4Base(a, tc, value, _FILE_, __LINE__, -1)
##endif

  #define structuredBufferAt(a, tc) a[uint((CHECK_STRUCTURED_BUFFER_EXPR(a, tc), tc))]
  #define bufferAt(a, tc) a[uint((CHECK_BUFFER_EXPR(a, tc), tc))]
  #define texture2DAt(a, tc) a[int2((CHECK_TEXTURE2D_EXPR(a, tc), tc))]
  #define texture3DAt(a, tc) a[int3((CHECK_TEXTURE3D_EXPR(a, tc), tc))]
#endif

##if (hardware.xbox && !hardware.dx12) || hardware.vulkan
  uint stencilFetchBase(Texture2D<uint2> a, int2 tc, int file, int ln, int name) { CHECK_STENCIL; return a[tc].r; }
##else
  uint stencilFetchBase(Texture2D<uint2> a, int2 tc, int file, int ln, int name) { CHECK_STENCIL; return a[tc].g; }
##endif
  #define stencilFetch(a, tc) stencilFetchBase(a, tc, _FILE_, __LINE__, get_name_##a)
}

hlsl {
  half3 h3nanofilter(half3 val)
  {
##if hardware.metal
  #if HALF_PRECISION
    return min(val, 65504.h); // clamp to half max
  #elif SHADER_COMPILER_DXC
    return isfinite(dot(val, val)) ?  val : half3(0, 0, 0);
  #else
    return isfinite(dot(val, val)).xxx ? val : half3(0, 0, 0);
  #endif
##else
  #if HALF_PRECISION
    return min(val, 65504.h); // clamp to half max
  #elif SHADER_COMPILER_HLSL2021
    return select(isfinite(dot(val, val)).xxx, val, half3(0, 0, 0));
  #else
    return isfinite(dot(val, val)).xxx ? val : half3(0, 0, 0);
  #endif
    // -min(-val, 0) does not work on DX11.
##endif
  }
}

macro USE_SAMPLE_BICUBIC()
hlsl(ps)
{
  #define SAMPLE_BICUBIC_LOD0(tex, tex_sizes, in_uv, res) { \
    float2 uv = in_uv; uv *= (tex_sizes).xy; float2 tc = floor(uv.xy - 0.5) + 0.5; \
    float2 f = uv.xy-tc; float2 f2 = f * f; float2 f3 = f2 * f; \
    float2 w0 = f2 - 0.5 * (f3 + f); float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.; float2 w3 = 0.5 * (f3-f2); float2 w2 = 1. - w0 - w1 - w3; \
    float2 s0 = w0 + w1; float2 s1 = w2 + w3; \
    float2 f0 = w1 / s0; float2 f1 = w3 / s1; \
    float2 t0 = tc - 1. + f0; float2 t1 = tc + 1. + f1; \
    t0 *= (tex_sizes).zw; t1 *= (tex_sizes).zw; \
    res = tex2Dlod(tex, float4(t0, 0, 0)) * s0.x * s0.y \
    + tex2Dlod(tex, float4(t1.x, t0.y, 0, 0)) * s1.x * s0.y \
    + tex2Dlod(tex, float4(t0.x, t1.y, 0, 0)) * s0.x * s1.y \
    + tex2Dlod(tex, float4(t1, 0, 0)) * s1.x * s1.y; }
}
endmacro

hlsl(hs)
{
  ##if hardware.ps4 || hardware.ps5
    #define domain DOMAIN_PATCH_TYPE
    #define outputtopology OUTPUT_TOPOLOGY_TYPE
    #define outputcontrolpoints OUTPUT_CONTROL_POINTS
    #define partitioning PARTITIONING_TYPE
    #define patchconstantfunc PATCH_CONSTANT_FUNC

    #define SV_OutputControlPointID S_OUTPUT_CONTROL_POINT_ID
    #define SV_TessFactor S_EDGE_TESS_FACTOR
    #define SV_InsideTessFactor S_INSIDE_TESS_FACTOR
    #define DECLARE_MAX_TESS_FACTOR(n) [MAX_TESS_FACTOR(n)]
  ##else
    #define DECLARE_MAX_TESS_FACTOR(n)
  ##endif
}
hlsl(ds)
{
  ##if hardware.ps4 || hardware.ps5
    #define domain DOMAIN_PATCH_TYPE
    #define SV_TessFactor S_EDGE_TESS_FACTOR
    #define SV_InsideTessFactor S_INSIDE_TESS_FACTOR
    #define SV_DomainLocation S_DOMAIN_LOCATION
  ##endif
}
