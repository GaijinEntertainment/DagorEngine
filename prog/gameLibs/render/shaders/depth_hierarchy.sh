include "shader_global.sh"

int downsample_op = 0;
interval downsample_op: mip_min<1, mip_max;

int required_mip_count;
int work_group_count;

int g_global_counter_no = 12;

shader depth_hierarchy
{
  // setup
  if (downsample_op == mip_min)
  {
    hlsl {
      #define REDUCE_OP min
      #define COUNTER_INDEX 0
    }
  } else if (downsample_op == mip_max)
  {
    hlsl {
      #define REDUCE_OP max
      #define COUNTER_INDEX 1
    }
  }

  if (hardware.xbox || hardware.ps4)
  {
    hlsl {
      // Xbox produces buggy results with wave-based quad reduction
      #define DISABLE_WAVE_INTRINSICS 1
    }
  }

  USE_PS5_WAVE32_MODE()

  (cs) {
    mips_and_group_count@i2 = (required_mip_count, work_group_count);

    // TODO: try RWByteAddressBuffer instead
    if (hardware.ps5) {
      g_global_counter@uav : register(g_global_counter_no) hlsl {
        RW_RegularBuffer<uint, CacheFlags::kGL2Only> g_global_counter@uav; //atomic access, should be fine
      }
    }
    else if (hardware.ps4)
    {
      g_global_counter@uav : register(g_global_counter_no) hlsl {
        RW_RegularBuffer<uint> g_global_counter@uav;
      }
    }
    else
    {
      g_global_counter@uav : register(g_global_counter_no) hlsl {
        globallycoherent RWStructuredBuffer<uint> g_global_counter@uav;
      }
    }
  }

  hlsl(cs) {
    #include <depth_hierarchy_inc.hlsli>

    Texture2D<float> g_depth_buffer : register(t0);
    RWTexture2D<float> g_downsampled_depth_buffer[12] : register(u0);

    //
    // Taken from https://github.com/GPUOpen-Effects/FidelityFX-SPD
    //

    float ReduceSrcDepth4(uint2 base)
    {
      float v0 = g_depth_buffer[base + uint2(0, 0)];
      float v1 = g_depth_buffer[base + uint2(0, 1)];
      float v2 = g_depth_buffer[base + uint2(1, 0)];
      float v3 = g_depth_buffer[base + uint2(1, 1)];
      return REDUCE_OP(REDUCE_OP(v0, v1), REDUCE_OP(v2, v3));
    }

#if DISABLE_WAVE_INTRINSICS
    groupshared float g_groupshared_depth_tmp[16][16];
#endif

    void WriteDstDepth(uint index, uint2 icoord, float v)
    {
      g_downsampled_depth_buffer[index][icoord] = v;
    }
    float ReduceQuad(float v, uint2 xy)
    {
#if DISABLE_WAVE_INTRINSICS
      g_groupshared_depth_tmp[xy.x][xy.y] = v;
      GroupMemoryBarrierWithGroupSync();
      uint2 xy0 = xy & (~0x1);
      float v0 = g_groupshared_depth_tmp[xy0.x | 0][xy0.y | 0];
      float v1 = g_groupshared_depth_tmp[xy0.x | 1][xy0.y | 0];
      float v2 = g_groupshared_depth_tmp[xy0.x | 0][xy0.y | 1];
      float v3 = g_groupshared_depth_tmp[xy0.x | 1][xy0.y | 1];
#else
      uint quad = WaveGetLaneIndex() & (~0x3); // zero out the last two bits
      float v0 = v;
      float v1 = WaveReadLaneAt(v, quad | 1); // value from horizontal neighbour
      float v2 = WaveReadLaneAt(v, quad | 2); // value from vertical neighbour
      float v3 = WaveReadLaneAt(v, quad | 3); // value from diagonal neighbour
#endif
      return REDUCE_OP(REDUCE_OP(v0, v1), REDUCE_OP(v2, v3));
    }
    groupshared float g_groupshared_depth[16][16];
    groupshared uint g_groupshared_counter;

    float ReduceLoad4(int2 base)
    {
      float v0 = g_downsampled_depth_buffer[5][base + int2(0, 0)];
      float v1 = g_downsampled_depth_buffer[5][base + int2(0, 1)];
      float v2 = g_downsampled_depth_buffer[5][base + int2(1, 0)];
      float v3 = g_downsampled_depth_buffer[5][base + int2(1, 1)];
      return REDUCE_OP(REDUCE_OP(v0, v1), REDUCE_OP(v2, v3));
    }

    uint bitfieldExtract(uint src, uint off, uint bits){ uint mask=(1U<<bits)-1;return (src>>off)&mask; }
    uint bitfieldInsert(uint src, uint ins, uint bits){ uint mask=(1U<<bits)-1;return (ins&mask)|(src&(~mask)); }

    void DownsampleNext4Levels(int base_level, int GI, uint2 work_group_id, uint x, uint y)
    {
      uint2 xy = uint2(x,y);
      BRANCH
      if (mips_and_group_count.x <= base_level)
        return;
      // Init mip level 2 or 8
      float v = ReduceQuad(g_groupshared_depth[x][y], xy);
      // quad index 0 stores result
      if ((GI % 4) == 0)
      {
        WriteDstDepth(base_level, uint2(work_group_id * 8) + uint2(x / 2, y / 2), v);
        g_groupshared_depth[x + (y / 2) % 2][y] = v;
      }
      GroupMemoryBarrierWithGroupSync();

      BRANCH
      if (mips_and_group_count.x <= base_level + 1)
        return;
      // Init mip level 3 or 9
      if (GI < 64)
      {
          float v = ReduceQuad(g_groupshared_depth[x * 2 + y % 2][y * 2], xy);
          // quad index 0 stores result
          if ((GI % 4) == 0)
          {
            WriteDstDepth(base_level + 1, uint2(work_group_id * 4) + uint2(x / 2, y / 2), v);
            g_groupshared_depth[x * 2 + y / 2][y * 2] = v;
          }
      }
      GroupMemoryBarrierWithGroupSync();

      BRANCH
      if (mips_and_group_count.x <= base_level + 2)
        return;
      // Init mip level 4 or 10
      if (GI < 16)
      {
          float v = ReduceQuad(g_groupshared_depth[x * 4 + y][y * 4], xy);
          // quad index 0 stores result
          if ((GI % 4) == 0)
          {
            WriteDstDepth(base_level + 2, uint2(work_group_id * 2) + uint2(x / 2, y / 2), v);
            g_groupshared_depth[x / 2 + y][0] = v;
          }
      }
      GroupMemoryBarrierWithGroupSync();

      BRANCH
      if (mips_and_group_count.x <= base_level + 3)
        return;
      // Init mip level 5 or 11
      if (GI < 4)
      {
        float v = ReduceQuad(g_groupshared_depth[GI][0], xy);
        // quad index 0 stores result
        if ((GI % 4) == 0)
        {
          WriteDstDepth(base_level + 3, uint2(work_group_id), v);
        }
      }
      GroupMemoryBarrierWithGroupSync();
    }

    [numthreads(32, 8, 1)]
    void depth_hierarchy_cs( uint3 Groupid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex )
    {
      //
      // Remap index for easier reduction (arrange in nested quads)
      //
      //  00 01 02 03 04 05 06 07           00 01 08 09 10 11 18 19
      //  08 09 0a 0b 0c 0d 0e 0f           02 03 0a 0b 12 13 1a 1b
      //  10 11 12 13 14 15 16 17           04 05 0c 0d 14 15 1c 1d
      //  18 19 1a 1b 1c 1d 1e 1f   ---->   06 07 0e 0f 16 17 1e 1f
      //  20 21 22 23 24 25 26 27           20 21 28 29 30 31 38 39
      //  28 29 2a 2b 2c 2d 2e 2f           22 23 2a 2b 32 33 3a 3b
      //  30 31 32 33 34 35 36 37           24 25 2c 2d 34 35 3c 3d
      //  38 39 3a 3b 3c 3d 3e 3f           26 27 2e 2f 36 37 3e 3f

      uint sub_64 = uint(GI % 64);
      uint2 sub_8x8 = uint2(bitfieldInsert(bitfieldExtract(sub_64, 2, 3), sub_64, 1),
                            bitfieldInsert(bitfieldExtract(sub_64, 3, 3), bitfieldExtract(sub_64, 1, 2), 2));
      uint x = sub_8x8.x + 8 * ((GI / 64) % 2);
      uint y = sub_8x8.y + 8 * ((GI / 64) / 2);

      { // Init mip levels 0 and 1
        float v[4];
        uint2 icoord;

        icoord = uint2(Groupid.xy * 32) + uint2(x, y);
        v[0] = ReduceSrcDepth4(icoord * 2);
        WriteDstDepth(0, icoord, v[0]);

        icoord = uint2(Groupid.xy * 32) + uint2(x + 16, y);
        v[1] = ReduceSrcDepth4(icoord * 2);
        WriteDstDepth(0, icoord, v[1]);

        icoord = uint2(Groupid.xy * 32) + uint2(x, y + 16);
        v[2] = ReduceSrcDepth4(icoord * 2);
        WriteDstDepth(0, icoord, v[2]);

        icoord = uint2(Groupid.xy * 32) + uint2(x + 16, y + 16);
        v[3] = ReduceSrcDepth4(icoord * 2);
        WriteDstDepth(0, icoord, v[3]);

        BRANCH
        if (mips_and_group_count.x <= 1)
          return;

        uint2 xy = uint2(x,y);
        v[0] = ReduceQuad(v[0], xy);
        v[1] = ReduceQuad(v[1], xy);
        v[2] = ReduceQuad(v[2], xy);
        v[3] = ReduceQuad(v[3], xy);

        if ((GI % 4) == 0)
        {
          WriteDstDepth(1, uint2(Groupid.xy * 16) + uint2(x / 2, y / 2), v[0]);
          g_groupshared_depth[x / 2][y / 2] = v[0];

          WriteDstDepth(1, uint2(Groupid.xy * 16) + uint2(x / 2 + 8, y / 2), v[1]);
          g_groupshared_depth[x / 2 + 8][y / 2] = v[1];

          WriteDstDepth(1, uint2(Groupid.xy * 16) + uint2(x / 2, y / 2 + 8), v[2]);
          g_groupshared_depth[x / 2][y / 2 + 8] = v[2];

          WriteDstDepth(1, uint2(Groupid.xy * 16) + uint2(x / 2 + 8, y / 2 + 8), v[3]);
          g_groupshared_depth[x / 2 + 8][y / 2 + 8] = v[3];
        }
        GroupMemoryBarrierWithGroupSync();
      }

      DownsampleNext4Levels(2, GI, Groupid.xy, x, y);

      BRANCH
      if (mips_and_group_count.x <= 6)
        return;

      // Only the last active workgroup should proceed
      if (GI == 0)
        InterlockedAdd(g_global_counter[COUNTER_INDEX], 1, g_groupshared_counter);
      GroupMemoryBarrierWithGroupSync();
      BRANCH
      if (g_groupshared_counter != mips_and_group_count.y - 1)
        return;
      // Reset counter
      g_global_counter[COUNTER_INDEX] = 0;

      { // Init mip levels 6 and 7
        float v[4];
        int2 icoord;

        icoord = int2(x * 2 + 0, y * 2 + 0);
        v[0] = ReduceLoad4(icoord * 2);
        WriteDstDepth(6, icoord, v[0]);

        icoord = int2(x * 2 + 1, y * 2 + 0);
        v[1] = ReduceLoad4(icoord * 2);
        WriteDstDepth(6, icoord, v[1]);

        icoord = int2(x * 2 + 0, y * 2 + 1);
        v[2] = ReduceLoad4(icoord * 2);
        WriteDstDepth(6, icoord, v[2]);

        icoord = int2(x * 2 + 1, y * 2 + 1);
        v[3] = ReduceLoad4(icoord * 2);
        WriteDstDepth(6, icoord, v[3]);

        BRANCH
        if (mips_and_group_count.x <= 7)
          return;

        float vv = REDUCE_OP(REDUCE_OP(v[0], v[1]), REDUCE_OP(v[2], v[3]));
        WriteDstDepth(7, int2(x, y), vv);
      }

      DownsampleNext4Levels(8, GI, uint2(0, 0), x, y);
    }
  }
  compile("target_cs", "depth_hierarchy_cs");
}
