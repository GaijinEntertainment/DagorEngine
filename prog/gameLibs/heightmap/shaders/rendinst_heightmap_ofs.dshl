texture rendinst_clipmap;
float4 rendinst_clipmap_world_to_hmap_tex_ofs;
float4 rendinst_clipmap_world_to_hmap_ofs;
float4 rendinst_clipmap_zn_zf;

macro INIT_RENDINST_HEIGHTMAP_OFS(stage)
  (stage) {
    rendinst_clipmap@smp2d = rendinst_clipmap;
    rendinst_clipmap_world_to_hmap_tex_ofs@f4 = rendinst_clipmap_world_to_hmap_tex_ofs;
    rendinst_clipmap_world_to_hmap_ofs@f4 = rendinst_clipmap_world_to_hmap_ofs;
    rendinst_clipmap_zn_zf@f2 = rendinst_clipmap_zn_zf;
  }
endmacro

macro USE_RENDINST_HEIGHTMAP_OFS(stage)
  hlsl(stage) {
    void apply_renderinst_hmap_ofs(float2 worldPosXZ, inout float groundHeight)
    {
##if rendinst_clipmap != NULL
      float2 tc = worldPosXZ * rendinst_clipmap_world_to_hmap_ofs.xy + rendinst_clipmap_world_to_hmap_ofs.zw;
      float depth = tex2Dlod(rendinst_clipmap, float4(tc - rendinst_clipmap_world_to_hmap_tex_ofs.xy, 0, 0)).x;
      depth = any(abs(tc * 2 - 1) > 1) ? 0 : depth;
      if (depth > 0)
      {
        float zn = rendinst_clipmap_zn_zf.x, zf = rendinst_clipmap_zn_zf.y;
        float offset = depth * (zf - zn) + zn;
        groundHeight = max(groundHeight, offset);
      }
##endif
    }
  }
endmacro