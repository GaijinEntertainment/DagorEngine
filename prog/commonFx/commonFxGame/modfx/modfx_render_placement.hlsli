#ifndef DAFX_MODFX_RENDER_PLACEMENT_HLSL
#define DAFX_MODFX_RENDER_PLACEMENT_HLSL


DAFX_INLINE
ModfxDeclRenderPlacementParams ModfxDeclRenderPlacementParams_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclRenderPlacementParams*)( buf + ofs );
#else
  ModfxDeclRenderPlacementParams pp;
  pp.terrain_only = dafx_load_1ui(buf, ofs);
  pp.placement_threshold = dafx_load_1f(buf, ofs);
  return pp;
#endif
}

DAFX_INLINE
BBox modfx_apply_placement_to_culling( DAFX_CREF(ModfxParentRenData) parent_rdata, BufferData_cref buf, float4_cref v )
{
  BBox bbox;
  bbox.bmin = float3(v.x - v.w, v.y - v.w, v.z - v.w);
  bbox.bmax = float3(v.x + v.w, v.y + v.w, v.z + v.w);

  if ( parent_rdata.mods_offsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS] )
  {
    ModfxDeclRenderPlacementParams pp = ModfxDeclRenderPlacementParams_load(buf, parent_rdata.mods_offsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS]);
    if ( pp.placement_threshold > 0 )
    {
      bbox.bmin.y -= pp.placement_threshold;
      bbox.bmax.y += pp.placement_threshold;
    }
  }

  return bbox;
}


#endif