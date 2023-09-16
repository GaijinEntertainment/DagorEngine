#ifndef DAFX_MODFX_TOROIDAL_SIM_HLSL
#define DAFX_MODFX_TOROIDAL_SIM_HLSL

void modfx_toroidal_movement_sim(ModfxParentSimData_cref parent_sdata, BufferData_cref buf, float3_cref emitter_pos, float3_ref io_pos)
{
  if (!parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_BOX])
    return;

  ModfxDeclPosInitBox pp = ModfxDeclPosInitBox_load(buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_BOX]);
  float3 tor_dims = float3(pp.dims.x < 0.0 ? 1 : 0, pp.dims.y < 0.0 ? 1 : 0, pp.dims.z < 0.0 ? 1 : 0);
  float3 plain_dims = float3(1, 1, 1) - tor_dims;
  pp.dims += plain_dims; // To exclude division by 0.
  float3 box_origin = pp.dims * 0.5 + pp.offset + emitter_pos;
  io_pos = (frac((io_pos - box_origin) / -pp.dims) * -pp.dims + box_origin) * tor_dims + io_pos * plain_dims;
}

#endif
