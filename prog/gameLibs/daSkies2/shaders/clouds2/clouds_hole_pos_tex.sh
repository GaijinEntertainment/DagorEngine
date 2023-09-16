texture clouds_hole_pos_tex;

macro USE_CLOUDS_HOLE_POS_TEX(code)
  (code) {
    clouds_hole_pos_tex@tex = clouds_hole_pos_tex hlsl {Texture2D<float4> clouds_hole_pos_tex@tex;};
  }
  hlsl(code) {
    float3 get_clouds_hole_pos_vec()
    {
      #if WTM
        return float3(0.0, 0.0, 0.0);
      #else
        return float3(clouds_hole_pos_tex[uint2(0, 0)].xy, 0).xzy;
      #endif
    }
  }
endmacro