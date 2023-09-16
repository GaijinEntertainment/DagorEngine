include "shader_global.sh"
include "frustum.sh"
include "vtex.sh"




int clipmap_clear_histogram_buf_rw_const_no = 6;
int clipmap_clear_tile_info_buf_rw_const_no = 7;
int clipmap_fill_histogram_buf_rw_const_no = 7;

hlsl
{
  uint calc_histogram_index(uint x, uint y, uint mip, uint ri_index)
  {
    return (mip << (TILE_WIDTH_BITS + TILE_WIDTH_BITS + MAX_RI_VTEX_CNT_BITS)) | (ri_index << (TILE_WIDTH_BITS + TILE_WIDTH_BITS)) | (y << TILE_WIDTH_BITS) | (x);
  }
}


macro USE_POSTFX()
  z_test = false;
  z_write = false;
  cull_mode = none;

  if (mobile_render == off)
  {
    supports global_frame;
  }

  POSTFX_VS(0)
endmacro


macro CLIPMAP_CLEAR_HISTOGRAM(stage)
  (stage) {
    clipmap_histogram_buf_rw@uav : register(clipmap_clear_histogram_buf_rw_const_no) hlsl {
      RWByteAddressBuffer clipmap_histogram_buf_rw@uav;
    }
    clipmap_tile_info_buf_rw@uav : register(clipmap_clear_tile_info_buf_rw_const_no) hlsl {
      RWByteAddressBuffer clipmap_tile_info_buf_rw@uav;
    }
  }

  hlsl(stage)
  {
    void clearHistogram(uint dtId)
    {
      if (dtId >= TEX_TOTAL_ELEMENTS)
        return;

      if (dtId == 0)
        storeBuffer(clipmap_tile_info_buf_rw, 0, 0);

      storeBuffer(clipmap_histogram_buf_rw, dtId*4, 0);
    }
  }
endmacro


shader clipmap_clear_histogram_ps
{
  ENABLE_ASSERT(ps)
  USE_POSTFX()

  CLIPMAP_CLEAR_HISTOGRAM(ps)

  hlsl(ps)
  {
    void render_ps(VsOutput input HW_USE_SCREEN_POS)
    {
      uint2 dtId = uint2(GET_SCREEN_POS(input.pos).xy);
      uint dtIdFlattened = dtId.y * (TILE_WIDTH * TEX_MIPS) + dtId.x;
      clearHistogram(dtIdFlattened);
    }
  }

  compile("target_ps", "render_ps");
}

shader clipmap_clear_histogram_cs
{
  ENABLE_ASSERT(cs)
  CLIPMAP_CLEAR_HISTOGRAM(cs)

  hlsl(cs)
  {
    [numthreads( 64, 1, 1)]
    void render_cs( uint dtId : SV_DispatchThreadID )
    {
      clearHistogram(dtId);
    }
  }

  compile("target_cs", "render_cs");
}



int clipmap_histogram_buf_rw_const_no = 7 always_referenced;
texture clipmap_feedback_tex;

int clipmap_tex_mips = 0;
interval clipmap_tex_mips: other_low<4, n_4<5, n_5<6, n_6<7, n_7<8, other_high;


macro CLIPMAP_FILL_HISTOGRAM(stage)
  ENABLE_ASSERT(stage)
  (stage)
  {
    clipmap_feedback_tex@tex2d = clipmap_feedback_tex;
    clipmap_tex_mips@i1 = clipmap_tex_mips;
    clipmap_histogram_buf_rw@uav : register(clipmap_fill_histogram_buf_rw_const_no) hlsl {
      RWByteAddressBuffer clipmap_histogram_buf_rw@uav;
    }
  }
  hlsl(stage)
  {
    void fillHistogram(uint2 dtId)
    {
      if (any(dtId >= uint2(FEEDBACK_WIDTH, FEEDBACK_HEIGHT)))
        return;

      uint4 rawFeedback = uint4(texelFetch(clipmap_feedback_tex, dtId, 0) * 255 + 0.5);
      uint2 pos = rawFeedback.xy;
      uint mip = rawFeedback.z;

#if MAX_RI_VTEX_CNT_BITS > 0
      uint ri_index = rawFeedback.w;
#else
      uint ri_index = 0;
#endif

      if ( all(pos < TILE_WIDTH && mip < clipmap_tex_mips && ri_index < MAX_VTEX_CNT) )
      {
        uint histId = calc_histogram_index(pos.x, pos.y, mip, ri_index);
        clipmap_histogram_buf_rw.InterlockedAdd(histId*4, 1u);
      }
    }
  }
endmacro


shader clipmap_fill_histogram_ps
{
  ENABLE_ASSERT(ps)
  USE_POSTFX()

  CLIPMAP_FILL_HISTOGRAM(ps)

  hlsl(ps)
  {
    void render_ps(VsOutput input HW_USE_SCREEN_POS)
    {
      uint2 dtId = uint2(GET_SCREEN_POS(input.pos).xy);
      fillHistogram(dtId);
    }
  }

  compile("target_ps", "render_ps");
}

shader clipmap_fill_histogram_cs
{
  ENABLE_ASSERT(cs)
  CLIPMAP_FILL_HISTOGRAM(cs)

  hlsl(cs)
  {
    [numthreads( 8, 8, 1)]
    void render_cs( uint2 dtId : SV_DispatchThreadID )
    {
      fillHistogram(dtId);
    }
  }

  compile("target_cs", "render_cs");
}





int clipmap_tile_info_buf_rw_const_no = 7 always_referenced;
buffer clipmap_histogram_buf;


macro BUILD_TILE_INFO(stage)
  (stage)
  {
    clipmap_histogram_buf@buf = clipmap_histogram_buf hlsl { ByteAddressBuffer clipmap_histogram_buf@buf; };
    clipmap_tex_mips@i1 = (clipmap_tex_mips);
    clipmap_tile_info_buf_rw@uav : register(clipmap_tile_info_buf_rw_const_no) hlsl {
      RWByteAddressBuffer clipmap_tile_info_buf_rw@uav;
    }
  }

  hlsl(stage)
  {
    static const uint4 BITS = uint4(TILE_PACKED_X_BITS, TILE_PACKED_Y_BITS, TILE_PACKED_MIP_BITS, TILE_PACKED_COUNT_BITS);
    static const uint4 SHIFT = uint4(0, BITS.x, BITS.x + BITS.y, BITS.x + BITS.y + BITS.z);
    static const uint4 MASK = (1U << BITS) - 1U;

    uint packTileInfo(uint x, uint y, uint mip, uint hist_cnt)
    {
      uint4 data = uint4(x, y, mip, hist_cnt);
      data = min(data, MASK); // Only count can be outside of bounds, but masking would make it overflow, so we use actual clamping. The rest is clamped as well for safety.
      data = data << SHIFT;
      return data.x | data.y | data.z | data.w;
    }

    void buildTileInfo(uint2 dtId)
    {
      if (any(dtId >= TILE_WIDTH))
        return;

##if clipmap_tex_mips == n_4
      #define UNROLLED_MIP_CNT 4
##elif clipmap_tex_mips == n_5
      #define UNROLLED_MIP_CNT 5
##elif clipmap_tex_mips == n_6
      #define UNROLLED_MIP_CNT 6
##elif clipmap_tex_mips == n_7
      #define UNROLLED_MIP_CNT 7
##endif

      UNROLL for (uint ri_index = 0; ri_index < MAX_VTEX_CNT; ++ri_index)
      {
#ifdef UNROLLED_MIP_CNT
      UNROLL for (uint mip = 0; mip < UNROLLED_MIP_CNT; ++mip)
#else
      for (uint mip = 0; mip < clipmap_tex_mips; ++mip)
#endif
      {
        uint histId = calc_histogram_index(dtId.x, dtId.y, mip, ri_index);
        uint histCnt = loadBuffer(clipmap_histogram_buf, histId*4);

        BRANCH
        if (histCnt > 0)
        {
          uint at;
          clipmap_tile_info_buf_rw.InterlockedAdd(0, 1u, at);
          ++at; // first entry is cnt

#if MAX_RI_VTEX_CNT_BITS > 0
          storeBuffer(clipmap_tile_info_buf_rw, (at*2 + 0)*4, packTileInfo(dtId.x, dtId.y, mip, histCnt));
          storeBuffer(clipmap_tile_info_buf_rw, (at*2 + 1)*4, ri_index);
#else
          storeBuffer(clipmap_tile_info_buf_rw, at*4, packTileInfo(dtId.x, dtId.y, mip, histCnt));
#endif
        }
      }
      }
    }
  }
endmacro


shader clipmap_build_tile_info_ps
{
  ENABLE_ASSERT(ps)
  USE_POSTFX()

  BUILD_TILE_INFO(ps)

  hlsl(ps)
  {
    void render_ps(VsOutput input HW_USE_SCREEN_POS)
    {
      uint2 dtId = uint2(GET_SCREEN_POS(input.pos).xy);
      buildTileInfo(dtId);
    }
  }

  compile("target_ps", "render_ps");
}


shader clipmap_build_tile_info_cs
{
  ENABLE_ASSERT(cs)
  BUILD_TILE_INFO(cs)

  hlsl(cs)
  {
    [numthreads( 8, 8, 1)]
    void render_cs( uint2 dtId : SV_DispatchThreadID )
    {
      buildTileInfo(dtId);
    }
  }

  compile("target_cs", "render_cs");
}
