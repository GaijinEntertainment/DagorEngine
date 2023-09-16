texture water_heightmap_grid;
texture water_heightmap_pages;
float4 water_height_offset_scale;
float4 world_to_water_heightmap;
float4 water_heightmap_page_count;
float water_heightmap_page_size;
float water_heightmap_det_scale;

macro INIT_WORLD_TO_WATER_HEIGHTMAP(stage)
  (stage) { world_to_water_heightmap@f4 = (world_to_water_heightmap); }
endmacro

macro INIT_WATER_HEIGHTMAP_BASE(stage, tex_or_sampler)
  INIT_WORLD_TO_WATER_HEIGHTMAP(stage)
  (stage) {
    water_heightmap_grid tex_or_sampler = water_heightmap_grid;
    water_heightmap_pages tex_or_sampler = water_heightmap_pages;
    water_height_offset_scale__page_padding@f4 = (water_height_offset_scale.x,
                water_height_offset_scale.y,
                water_heightmap_page_size / (water_heightmap_page_size + 2.0),
                1.0 / (water_heightmap_page_size + 2.0));
    water_heightmap_pages_count__scale@f4 = (water_heightmap_page_count.x,
                water_heightmap_page_count.y,
                water_heightmap_det_scale,
                0);
  }
endmacro

macro INIT_WATER_HEIGHTMAP(stage)
  INIT_WATER_HEIGHTMAP_BASE(stage, @tex2d)
endmacro

macro USE_WATER_HEIGHTMAP(stage)
  hlsl(stage) {
    #define water_heightmap_pages_count water_heightmap_pages_count__scale.xy
    #define water_heightmap_det_scale water_heightmap_pages_count__scale.z
    #define water_height_offset_scale water_height_offset_scale__page_padding.xy
    #define water_height_page_padding water_height_offset_scale__page_padding.zw
    float sample_water_height(float2 tc)
    {
        float height = -1.0;
      ##if water_heightmap_pages != NULL
        float2 tc_sat = saturate(tc);
        if (tc.x == tc_sat.x && tc.y == tc_sat.y)
        {
          uint2 dim;
          water_heightmap_grid.GetDimensions(dim.x, dim.y);
          float2 crd = dim * tc;
          uint value = water_heightmap_grid[crd].x * 0xFFFF;
          float2 gridPos = float2((value >> 1) & 0x7F, (value >> 8) & 0xFF);
          float scale = (value & 1) ? 1.0 : water_heightmap_det_scale;
          float2 innerOffset = frac(crd / scale) * water_height_page_padding.x + water_height_page_padding.y;
          if (value != 0xFFFF)
          {
            uint2 pages_dim;
            water_heightmap_pages.GetDimensions(pages_dim.x, pages_dim.y);
            height = tex2Dlod(water_heightmap_pages, float4((gridPos + innerOffset) * water_heightmap_pages_count + float2(0.5, 0.5) / pages_dim, 0, 0)).r;
          }
        }
      ##endif
      return height;
    }
    void get_water_height(float2 worldPosXZ, inout float height)
    {
      ##if water_heightmap_pages != NULL
        float2 tc = worldPosXZ * world_to_water_heightmap.zw + world_to_water_heightmap.xy;
        float h = sample_water_height(tc);
        if (h >= 0)
          height = h * water_height_offset_scale.y + water_height_offset_scale.x;
      ##endif
    }
  }
endmacro