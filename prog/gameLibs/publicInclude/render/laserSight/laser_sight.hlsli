struct LaserDecalInstance
{
  float4 pos_size;
  uint2 normal_du_isFps; //31 bits normal, 17 bit tangent(du), rest unused
  uint2 color;
  float3 origin_pos; uint pad;
};

struct LaserDecalsToUpdate
{
  float4 pos_size;
  float3 normal; uint decal_id;
  float3 right; uint is_from_fps_view; //7 bits texture id, 9 bit matrix id, 16 bits flags
  float4 color;
  float3 origin_pos; uint pad2;
};