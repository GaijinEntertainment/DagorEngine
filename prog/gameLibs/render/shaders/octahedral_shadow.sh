include "shader_global.sh"
include "postfx_inc.sh"

texture octahedral_temp_shadow;
float4 octahedral_texture_size = float4(1, 1, 0, 0);
float4 octahedral_shadow_zn_zfar = float4(1, 1, 0, 0);

shader octahedral_shadow_packer
{
  supports global_frame;
  cull_mode = none;
  z_test = true;
  z_write = true;
  no_ablend;

  POSTFX_VS_TEXCOORD(0, texcoord)
  ENABLE_ASSERT(ps)

  (ps)
  {
    octahedral_temp_shadow@texArray = octahedral_temp_shadow;
    octahedral_texture_size__margin@f4 = octahedral_texture_size;
    zn_zfar@f4 = (octahedral_shadow_zn_zfar.x, octahedral_shadow_zn_zfar.y, 1.0 / octahedral_shadow_zn_zfar.y, (octahedral_shadow_zn_zfar.y-octahedral_shadow_zn_zfar.x)/(octahedral_shadow_zn_zfar.x * octahedral_shadow_zn_zfar.y));
  }
  hlsl (ps)
  {

    #include <octahedral.hlsl>


    static const float2 cosSin[3] =
    {
      float2(0, 1), float2(-1, 0), float2(0, -1)
    };

    float octahedral_shadow_packer_ps(VsOutput input) : SV_Depth
    {
      // center of the texture: (0, 1, 0)
      float2 extent = octahedral_texture_size__margin.xy;
      float2 relMargin = octahedral_texture_size__margin.zw; // predevided by extent
      float2 octahedralTc = (input.texcoord - relMargin) / (1 - 2*relMargin);
      if (octahedralTc.x < 0)
        octahedralTc = float2(-octahedralTc.x, 1-octahedralTc.y);
      else if (octahedralTc.x > 1)
        octahedralTc = float2(2-octahedralTc.x, 1-octahedralTc.y);
      if (octahedralTc.y < 0)
        octahedralTc = float2(1-octahedralTc.x, -octahedralTc.y);
      else if (octahedralTc.y > 1)
        octahedralTc = float2(1-octahedralTc.x, 2-octahedralTc.y);

      float3 dir = octDecode(octahedralTc * 2 - 1).xzy;
      uint viewId = 0;
      float maxXZ = max(abs(dir.x), abs(dir.z));

      // [forward, right, backward, left, up, down]
      if (dir.y > maxXZ) {
        viewId = 4;
        dir.yz = float2(-dir.z, dir.y);
      }
      else if (dir.y < -maxXZ) {
        viewId = 5;
        dir.yz = float2(dir.z, -dir.y);
      }
      else {
        if (dir.z > abs(dir.x))
          viewId = 0;
        else
        {
          if (dir.z < -abs(dir.x))
            viewId = 2;
          else
            viewId = dir.x > 0 ? 1 : 3;
            float2 c_s = cosSin[viewId-1];
            dir = float3(c_s.x*dir.x - c_s.y*dir.z, dir.y, c_s.y*dir.x + c_s.x*dir.z);
        }
      }
      dir.y *= -1;
      dir /= dir.z;
      int2 tc = clamp(
        int2((dir.xy * 0.5 + 0.5)*extent),
        int2(0, 0),
        int2(extent-1));

      float rawZ = texelFetch(octahedral_temp_shadow, int3(tc, viewId), 0).x;
      float linearZ = linearize_z(rawZ, zn_zfar.zw);
      float distance = linearZ * length(dir);
      return inv_linearize_distance(distance, zn_zfar);
    }
  }
  compile("target_ps", "octahedral_shadow_packer_ps");
}