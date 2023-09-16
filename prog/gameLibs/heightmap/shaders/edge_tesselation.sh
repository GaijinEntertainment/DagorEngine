macro DECODE_EDGE_TESSELATION()
  hlsl(vs) {
    float adapt_edge_tesselation(float y, float tess){return y - fmod(y,tess);}//still faster on current gpu (PS4), than y-y%tess uints
    //uint adapt_edge_tesselation(uint y, uint tess){return y - y%tess;}//still slower on current gpu (PS4), unless you use mul24 or bits
    uint4 decode_edge_tesselation(float4 edge_const)
    {
      uint edge_consti = (uint)(edge_const.y + 0.5);
      return (uint4(edge_consti&15, (edge_consti>>4)&15, (edge_consti>>8)&15, (edge_consti>>12)&15)) + 1;
    }
    float2 decodeWorldPosXZConst(float4 decConst, float2 inPos)
    {
      return decConst.x*inPos+decConst.zw;
    }
    float2 getGradXZ(float4 decConst, float2 tess)
    {
      return decConst.x*tess;
    }
    #define INPUT_VERTEXID_POSXZ uint vertexId : SV_VertexID
    #define DECODE_VERTEXID_POSXZ int2 posXZ = int2(vertexId%patchDimPlus1, vertexId/patchDimPlus1);
  }
endmacro

int heightmap_scale_offset = 70 always_referenced;

macro BASE_HEIGHTMAP_DECODE_EDGE_TESSELATION()
  DECODE_EDGE_TESSELATION()
  hlsl(vs) {
    float3 patchDim_patchDimPlus1_cellSize: register(c69);
    #define patchDim patchDim_patchDimPlus1_cellSize.x
    #define patchDimPlus1 int(patchDim_patchDimPlus1_cellSize.y)
    #define oneCellSize float(patchDim_patchDimPlus1_cellSize.z)

    #if NO_HW_INSTANCING
      float4 heightmap_scale_offset: register(c70);
      #define input_used_instance_id
      #define used_instance_id
      #define USED_INSTANCE_ID
      #define USE_INSTANCE_ID
    #else
      float4 heightmap_scale_offset[440]: register(c70);
      float4 lastreg:register(c511);
      ##if hardware.dx11 || hardware.ps4 || hardware.ps5 || hardware.vulkan || hardware.metal
         #define input_used_instance_id ,uint instance_id
         #define used_instance_id ,instance_id
         #define USED_INSTANCE_ID ,instance_id
         #define USE_INSTANCE_ID  ,uint instance_id:SV_InstanceID
      ##else
          #define input_used_instance_id ,int2 instance_id
          #define used_instance_id ,instance_id
          #define USED_INSTANCE_ID ,instance_id.x
          #define USE_INSTANCE_ID  ,int2 instance_id:TEXCOORD12
      ##endif
    #endif
  }
endmacro

macro HEIGHTMAP_CHANNELS()
endmacro

macro HEIGHTMAP_DECODE_EDGE_TESSELATION()
  HEIGHTMAP_CHANNELS()
  BASE_HEIGHTMAP_DECODE_EDGE_TESSELATION()
endmacro

macro PATCH_CONSTANT_DATA_CALCULATION()
hlsl(hs) {
  uint4 decodeTessFactor(uint encoded)
  {
    return uint4(encoded >> 24, (encoded >> 16) & 0xFF, (encoded >> 8) & 0xFF, encoded & 0xFF);
  }

  HsPatchData HullShaderPatchConstantCommon(OutputPatch<HsControlPoint, 4> controlPoints, float maxDisplacement)
  {
    HsPatchData patch;
    float3 worldNormal = float3(0,1,0);
    float3 positions[4] = {
      restoreWorldPos(controlPoints[0].pos),
      restoreWorldPos(controlPoints[1].pos),
      restoreWorldPos(controlPoints[2].pos),
      restoreWorldPos(controlPoints[3].pos)
    };
    float3 minPoint = min(min(positions[0], positions[1]), min(positions[2], positions[3]));
    float3 maxPoint = max(max(positions[0], positions[1]), max(positions[2], positions[3]));
    minPoint -= maxDisplacement.xxx;
    maxPoint += maxDisplacement.xxx;

    float4 dists = float4(
      length((positions[3] + positions[0]) * 0.5 - world_view_pos),
      length((positions[0] + positions[1]) * 0.5 - world_view_pos),
      length((positions[1] + positions[2]) * 0.5 - world_view_pos),
      length((positions[2] + positions[3]) * 0.5 - world_view_pos)
    );

    float max_dist = oneCellSize * patchDim * 3;
    dists = saturate(dists / max_dist);
    float4 distFade = lerp(1, hmap_tess_factor, pow2(float4(1,1,1,1) - dists));
#if CHECK_LOD0_VIGNETTE
    float3 centerPos = positions[0] + positions[1] + positions[2] + positions[3];
    distFade = lerp(1, distFade, float4(calc_lod0_vignette((positions[3] + positions[0]) * 0.5),
                                        calc_lod0_vignette((positions[0] + positions[1]) * 0.5),
                                        calc_lod0_vignette((positions[1] + positions[2]) * 0.5),
                                        calc_lod0_vignette((positions[2] + positions[3]) * 0.5)));
#endif

    float tessFactor = testBoxB(minPoint, maxPoint) ? hmap_tess_factor : 0;
#if FORCE_MIN_TESS_FACTOR_ON_EDGES
    uint4 patchTessFactor[4] = {
      decodeTessFactor(controlPoints[0].patchTessFactor),
      decodeTessFactor(controlPoints[1].patchTessFactor),
      decodeTessFactor(controlPoints[2].patchTessFactor),
      decodeTessFactor(controlPoints[3].patchTessFactor),
    };

    float4 minTessFactor = min(
      min(patchTessFactor[0], patchTessFactor[1]),
      min(patchTessFactor[2], patchTessFactor[3]));
#else
    float4 minTessFactor = hmap_tess_factor.xxxx;
#endif

    minTessFactor = min(min(tessFactor.xxxx, distFade), minTessFactor);

    patch.edges[0] = minTessFactor.x;
    patch.edges[1] = minTessFactor.y;
    patch.edges[2] = minTessFactor.z;
    patch.edges[3] = minTessFactor.w;
    patch.inside[0] = min(min(patch.edges[0], patch.edges[1]), min(patch.edges[2], patch.edges[3]));
    patch.inside[1] = patch.inside[0];
    return patch;
  }
}
endmacro