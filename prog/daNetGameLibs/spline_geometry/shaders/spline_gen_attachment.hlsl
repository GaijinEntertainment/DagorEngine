#ifndef _SPLINE_GEN_ATTACHMENT_
#define _SPLINE_GEN_ATTACHMENT_
//if you modify this, change ATTACHMENT_DATA_SIZE in spline_gen_buffer.hlsl

struct AttachmentData
{
  float4 tm0;
  float4 tm1;
  float4 tm2;
};
#define TM_LOOKUP(index) data.tm##index

void pack_tm(float3x3 rot_scale_mat, float3 base_world_pos, out AttachmentData data)
{
  TM_LOOKUP(0) = float4(rot_scale_mat[0], base_world_pos.x);
  TM_LOOKUP(1) = float4(rot_scale_mat[1], base_world_pos.y);
  TM_LOOKUP(2) = float4(rot_scale_mat[2], base_world_pos.z);
}

void unpack_tm(in AttachmentData data, out float3x3 rot_scale_mat, out float3 base_world_pos)
{
  rot_scale_mat[0] = TM_LOOKUP(0).xyz;
  rot_scale_mat[1] = TM_LOOKUP(1).xyz;
  rot_scale_mat[2] = TM_LOOKUP(2).xyz;
  base_world_pos = float3(TM_LOOKUP(0).w, TM_LOOKUP(1).w, TM_LOOKUP(2).w);
}

#undef TM_LOOKUP

#endif