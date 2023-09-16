buffer capsuled_units_ao;
buffer capsuled_units_indirection;
int capsuled_units_ao_count;
float4 capsuled_units_ao_world_to_grid_mul_add = (0,0,-1,-1);

macro USE_CAPSULE_AO(code)
  ENABLE_ASSERT(code)
  (code)
  {
    capsuled_units_ao@buf = capsuled_units_ao hlsl {#include <capsuledAOOcclusion.hlsli>
      StructuredBuffer<CapsuledAOUnit> capsuled_units_ao@buf;}
    capsuled_units_indirection@buf = capsuled_units_indirection hlsl { StructuredBuffer<uint> capsuled_units_indirection@buf;}
    capsuled_units_ao_world_to_grid_mul_add@f4 = capsuled_units_ao_world_to_grid_mul_add;
  }
  if (hardware.xbox)
  {
    hlsl(code) {
      #if WAVE_INTRINSICS//reduce divirgence, on SM6.0 and consoles, but on Xbox works only in Pixel Shaders 
        // todo: implement and use WaveActiveBitOr here
        // #define MERGE_MASK(m) WaveReadFirstLane( WaveAllBitOr( (m) ) )
      #endif
    }
  } else
  {
    hlsl(code) {
      #if WAVE_INTRINSICS//reduce divirgence, on SM6.0 and consoles, but on Xbox works only in Pixel Shaders 
        #define MERGE_MASK(m) WaveReadFirstLane( WaveAllBitOr( (m) ) )
      #endif
    }
  }
  hlsl(code) {
    #ifndef MERGE_MASK
      #define MERGE_MASK(m) m
    #endif
    #include <primitives\capsuleOcclusion.hlsl>

    float unitCapsuledOcclusion(CapsuledAOUnit unit, float3 worldPos, float3 worldNormal)
    {
      if (any(abs(worldPos - unit.boxCenter.xyz) > unit.boxSize.xyz+AO_AROUND_UNIT))//twice the size of box +1meters
        return 1;
      float occlusion = 1;
      float4 boxMin = unit.boxCenter - 0.5*unit.boxSize;
      float4 boxMax = boxMin + unit.boxSize;
      UNROLL
      for (uint i = 0; i < MAX_AO_CAPSULES; ++i)
      {
        float4 ptA_radius, ptB_opacity;
        #if AO_CAPSULES_PACK_BITS == 16
          uint4 ptAB = unit.capsules[i].ptARadius_ptB;
          ptA_radius = boxMin + (ptAB&0xFFFF)/65535.0*unit.boxSize;
          ptB_opacity = boxMin + (ptAB>>16)/65535.0*unit.boxSize;
        #else
          uint2 ptAB = unit.capsules[i].ptARadius_ptB;
          ptA_radius  = float4(ptAB&0xFF, (ptAB>>8)&0xFF)/255.0*unit.boxSize + boxMin;
          ptB_opacity = float4((ptAB>>16)&0xFF, ptAB>>24)/255.0*unit.boxSize + boxMin;
        #endif
        occlusion *= capOcclusionSquared( worldPos, worldNormal, ptA_radius.xyz, ptB_opacity.xyz, ptA_radius.w);
      }
      return occlusion;
    }
    float getCapsulesOcclusion(float currentAO, float3 worldPos, float3 worldNormal)
    {
      int2 gridPos = floor(capsuled_units_ao_world_to_grid_mul_add.xy*worldPos.xz + capsuled_units_ao_world_to_grid_mul_add.zw);
      BRANCH
      if (any(gridPos < 0 || gridPos >= UNITS_AO_GRID_SIZE) || currentAO<0.01)//todo: check if it is outside meaningful grid, not maximum grid. i.e. maxium is something smaller.
        return currentAO;

      float occlusion = currentAO;
      int gridIndex = gridPos.x + gridPos.y*UNITS_AO_GRID_SIZE;
      uint mask = structuredBufferAt(capsuled_units_indirection, gridIndex);
      uint mergedMask = MERGE_MASK( mask );
      while (mergedMask != 0) // processed per lane// && occlusion>0.001
      {
        uint bitIndex = firstbitlow( mergedMask );
        mergedMask ^= ( 1U << bitIndex );
        occlusion *= unitCapsuledOcclusion(structuredBufferAt(capsuled_units_ao, bitIndex), worldPos, worldNormal);
      }

      return occlusion;
    }
  }
endmacro