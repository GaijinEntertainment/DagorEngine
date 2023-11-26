macro CLEAR_TYPE_VOLMAP_CS(type, shader_name)
shader shader_name
{
  hlsl(cs) {
    RWTexture3D<type> volume_map : register(u7);
    [numthreads(4, 4, 4)]
    void clear_volmap_cs( uint3 dtId : SV_DispatchThreadID )//uint3 gId : SV_GroupId,
    {
      volume_map[dtId] = 0;
    }
  }
  compile("cs_5_0", "clear_volmap_cs");
}
endmacro

CLEAR_TYPE_VOLMAP_CS(float, clear_float_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(float3, clear_float3_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(float4, clear_float4_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(uint, clear_uint_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(uint2, clear_uint2_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(uint3, clear_uint3_volmap_cs)
CLEAR_TYPE_VOLMAP_CS(uint4, clear_uint4_volmap_cs)