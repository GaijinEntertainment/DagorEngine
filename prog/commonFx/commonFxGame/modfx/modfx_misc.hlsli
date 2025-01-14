#ifndef DAFX_MODFX_MISC_HLSL
#define DAFX_MODFX_MISC_HLSL

float3 normalize_safe( float3 v, float3 def )
{
  float l = length( v );
  return l > 0 ? v / l : def;
}

float2 normalize_safe( float2 v, float2 def )
{
  float l = length( v );
  return l > 0 ? v / l : def;
}

float2 normalize_safe_0( float2 v )
{
  return normalize_safe( v, float2( 0, 0 ) );
}

float3 normalize_safe_0( float3 v )
{
  return normalize_safe( v, float3( 0, 0, 0 ) );
}

ModfxColorEmission ModfxColorEmission_load( BufferData_cref buf, uint ofs )
{
  ModfxColorEmission pp;
  pp.mask = dafx_load_1ui( buf, ofs );
  pp.value = dafx_load_1f( buf, ofs );
  return pp;
};

ModfxDepthMask ModfxDepthMask_load( BufferData_cref buf, uint ofs )
{
  ModfxDepthMask pp;
  pp.depth_softness_rcp = dafx_load_1f( buf, ofs );
  pp.znear_softness_rcp = dafx_load_1f( buf, ofs );
  pp.znear_clip_offset = dafx_load_1f( buf, ofs );
  return pp;
}

ModfxDeclShapeStaticAligned ModfxDeclShapeStaticAligned_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclShapeStaticAligned pp;
  pp.cross_fade_mul = dafx_load_1f( buf, ofs );
  pp.cross_fade_pow = dafx_load_1ui( buf, ofs );
  pp.cross_fade_threshold = dafx_load_1f( buf, ofs );
  return pp;
}

ModfxDeclShapeStaticAlignedInit ModfxDeclShapeStaticAlignedInit_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclShapeStaticAlignedInit pp;
  pp.up_vec = dafx_load_3f( buf, ofs );
  pp.right_vec = dafx_load_3f( buf, ofs );
  return pp;
}

ModfxDeclExternalOmnilight ModfxDeclExternalOmnilight_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclExternalOmnilight*)( buf + ofs );
#else
  ModfxDeclExternalOmnilight pp;
  pp.pos = dafx_load_3f( buf, ofs );
  float4 col_and_rad = dafx_load_4f( buf, ofs );
  pp.color = col_and_rad.xyz;
  pp.radius = col_and_rad.w;
  return pp;
#endif
}

ModfxDeclLighting ModfxDeclLighting_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclLighting pp;
  uint4 v = unpack_4b( dafx_load_1ui( buf, ofs ) );
  pp.type = v.x;
  pp.translucency = v.y;
  pp.normal_softness = v.z;
  pp.specular_power = v.w;

  v = unpack_4b( dafx_load_1ui( buf, ofs ) );
  pp.specular_strength = v.x;
  pp.sphere_normal_power = v.y;
  pp.sphere_normal_radius = dafx_load_1f( buf, ofs );
  return pp;
}

ModfxDeclDistortionFadeColorColorStrength ModfxDeclDistortionFadeColorColorStrength_load(BufferData_cref buf, uint ofs)
{
  ModfxDeclDistortionFadeColorColorStrength pp;

  pp.fadeRange     = dafx_load_1f(buf, ofs);
  pp.fadePower     = dafx_load_1f(buf, ofs);
  pp.color         = dafx_load_1ui(buf, ofs);
  pp.colorStrength = dafx_load_1f(buf, ofs);

  return pp;
}

ModfxDeclRibbonParams ModfxDeclRibbonParams_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclRibbonParams pp;
  pp.uv_tile = dafx_load_1ui( buf, ofs );
  pp.side_fade_params = dafx_load_2f( buf, ofs );
  pp.head_fade_params = dafx_load_2f( buf, ofs );
  return pp;
}

DAFX_INLINE
ModfxDeclServiceTrail ModfxDeclServiceTrail_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclServiceTrail*)( buf + ofs );
#else
  ModfxDeclServiceTrail pp;
  pp.last_emitter_pos = dafx_load_3f( buf, ofs );
  pp.prev_last_emitter_pos = dafx_load_3f( buf, ofs );
  pp.flags = dafx_load_1ui( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void ModfxDeclServiceTrail_save( DAFX_CREF(ModfxDeclServiceTrail) v, BufferData_ref buf, uint ofs )
{
  dafx_store_3f( v.last_emitter_pos, buf, ofs );
  dafx_store_3f( v.prev_last_emitter_pos, buf, ofs );
  dafx_store_1ui( v.flags, buf, ofs );
}

DAFX_INLINE
ModfxDeclServiceUniqueId ModfxDeclServiceUniqueId_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclServiceUniqueId*)( buf + ofs );
#else
  ModfxDeclServiceUniqueId pp;
  pp.particles_emitted = dafx_load_1ui( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void ModfxDeclServiceUniqueId_save( DAFX_CREF(ModfxDeclServiceUniqueId) v, BufferData_ref buf, uint ofs )
{
  dafx_store_1ui( v.particles_emitted, buf, ofs );
}

DAFX_INLINE
ModfxDeclFrameInfo ModfxDeclFrameInfo_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclFrameInfo*)( buf + ofs );
#else
  ModfxDeclFrameInfo pp;
  pp.frames_x = dafx_load_1ui( buf, ofs );
  pp.frames_y = dafx_load_1ui( buf, ofs );
  pp.boundary_id_offset = dafx_load_1ui( buf, ofs );
  pp.inv_scale = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

ModfxDeclVolfogInjectionParams ModfxDeclVolfogInjectionParams_load(BufferData_cref buf, uint ofs)
{
  ModfxDeclVolfogInjectionParams pp;

  pp.weight_rgb = dafx_load_1f(buf, ofs);
  pp.weight_alpha = dafx_load_1f(buf, ofs);

  return pp;
}

#endif