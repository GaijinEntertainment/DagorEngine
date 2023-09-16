include "viewVecVS.sh"
include "frustum.sh"
include "dagi_scene_voxels_common.sh"
include "dagi_scene_common_write.sh"
include "dagi_volmap_gi.sh"
include "dagi_helpers.sh"
include "dagi_light_helpers.sh"

int ssgi_current_frame;
int has_physobj_in_cascade;
float4 ssgi_restricted_update_bbox_min;
float4 ssgi_restricted_update_bbox_max;

hlsl {
  #include "dagi_scene_common_types.hlsli"
  #include "dagi_voxels_consts.hlsli"

  #define SCENE_RES_BITS 8
  #define SCENE_RES_Z_BITS 6
  #define CASCADE_BITS 2
  #define SCENE_Z_SHIFT (SCENE_RES_BITS*2)
  #define CASCADE_SHIFT (SCENE_Z_SHIFT+SCENE_RES_Z_BITS)
  #define CASCADE_MASK ((1U<<CASCADE_BITS)-1)
  #define BIN_SHIFT (CASCADE_SHIFT+CASCADE_BITS)
  #define SCENE_RES_MASK ((1U<<SCENE_RES_BITS)-1)
  #define SCENE_RES_Z_MASK ((1U<<SCENE_RES_Z_BITS)-1)

  #define OLD_ALPHA_BITS (32-BIN_SHIFT)
  #define OLD_ALPHA_DIAP (1U<<OLD_ALPHA_BITS)
  #define OLD_ALPHA_MAX_VALUE (OLD_ALPHA_DIAP-1)
  uint encode_scene_voxel_coord_bin(uint3 coord, uint cascade, uint bin)
  {
    return coord.x | (coord.y<<SCENE_RES_BITS) | (coord.z<<SCENE_Z_SHIFT) | (cascade<<CASCADE_SHIFT) | (bin<<BIN_SHIFT);
  }
  uint3 decode_scene_voxel_coord_safe(uint voxel, out uint cascade, out uint bin)
  {
    cascade  = (voxel>>CASCADE_SHIFT)&CASCADE_MASK;
    bin = voxel>>BIN_SHIFT;
    return uint3(voxel&SCENE_RES_MASK, (voxel>>SCENE_RES_BITS)&SCENE_RES_MASK, (voxel>>SCENE_Z_SHIFT)&SCENE_RES_Z_MASK);
  }
  #if SCENE_VOXELS_COLOR == SCENE_VOXELS_SRGB8_A8
    uint encode_scene_temp_color(float3 col) {col=saturate(col)*255; return uint(col.r)|(uint(col.g)<<8)|(uint(col.b)<<16);}
    float3 decode_scene_temp_color(uint col) {return float3((col&0xFF),((col>>8)&0xFF),((col>>16)&0xFF))*1./255.;}

  #elif SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
    #include <pixelPacking/PixelPacking_R11G11B10.hlsl>
    uint encode_scene_temp_color(float3 col) {return Pack_R11G11B10_FIXED_LOG(col);}//Pack_R11G11B10_FLOAT//Pack_R11G11B10_FLOAT_LOG//Pack_R11G11B10_FIXED_LOG
    float3 decode_scene_temp_color(uint col) {return Unpack_R11G11B10_FIXED_LOG(col);}//Unpack_R11G11B10_FLOAT//Unpack_R11G11B10_FLOAT_LOG//
  #endif
}


float4x4 globtm_inv;

macro USE_AND_INIT_INV_GLOBTM(code)
  (code) { globtm_inv@f44 = globtm_inv; }
endmacro

define_macro_if_not_defined LIGHT_VOXELS_SCENE_HELPER(code)
  hlsl(code) {
    int get_voxel_scene_gbuffer_color(float2 tc, float w, float3 worldPos, out float3 next_color, out float next_alpha, int3 baseCoord, float3 voxel_size, uint cascade)
    {
      return 0;
    }
  }
endmacro

define_macro_if_not_defined USE_EMISSION_DECODE_COLOR_MAP(code)
endmacro

int ssgi_total_scene_mark_dipatch = 8192;

shader ssgi_mark_scene_voxels_from_gbuf_cs
{
  //todo: we can feed that to chanined compute shader in order to blend into current voxels with some weight

  INIT_ZNZFAR_STAGE(cs)
  USE_AND_INIT_INV_GLOBTM(cs)
  USE_EMISSION_DECODE_COLOR_MAP(cs)

  INIT_VOXELS(cs)
  USE_VOXELS(cs)
  SAMPLE_VOXELS(cs)
  ENABLE_ASSERT(cs)
  //USE_AND_INIT_VIEW_VEC_CS()
  (cs) {
    ssgi_current_frame@f2 = (ssgi_current_frame*ssgi_total_scene_mark_dipatch,ssgi_total_scene_mark_dipatch,0,0);
    world_view_pos@f3 = world_view_pos;
    has_physobj_in_cascade@i1 = (has_physobj_in_cascade);
    ssgi_restricted_update_bbox_min@f3 = (ssgi_restricted_update_bbox_min);
    ssgi_restricted_update_bbox_max@f3 = (ssgi_restricted_update_bbox_max);
  }
  hlsl(cs) {
    #define FILTER_BY_NORMAL_WEIGHT 0.05
    #define HARD_GI_VIGNETTE 1
  }

  //start lighting
  SSGI_USE_VOLMAP_GI(cs)
  LIGHT_VOXELS_SCENE_HELPER(cs)
  //end

  INIT_LOAD_DEPTH_GBUFFER_BASE(cs)
  USE_LOAD_DEPTH_GBUFFER_BASE(cs)

  hlsl(cs) {
    #include <pcg_hash.hlsl>
    #include <dagi_brigthnes_lerp.hlsl>
    RWByteAddressBuffer dispatchIndirectBuffer : register( u0 );
    RWStructuredBuffer<GBUF_VOXEL> gbuf_voxels: register(u1);

    float randPos(float3 p)
    {
      return frac(sin(dot(p.xyz, float3(1, -2.7813, 3.01))*12.9898) * 43758.5453);
    }
    float rand(float p)
    {
      return frac(sin(p*12.9898) * 43758.5453);
    }
    bool isInsideBbox(float3 pos, float3 bmin, float3 bmax)
    {
      return all(pos < bmax) && all(pos > bmin);
    }

    [numthreads(MARK_VOXELS_WARP_SIZE, 1, 1)]
    void mark_voxels_cs( uint dtId : SV_DispatchThreadID )
    {
      uint2 dim = gbuffer_depth_size_load;
      uint random = pcg_hash(dtId + ssgi_current_frame.x);
      uint2 tcI = uint2(random%dim.x, (random/dim.x)%dim.y);
      if (dim.x*dim.y <= ssgi_current_frame.y)
      //we assume we render envi cube face. In that case, we also assume that total groups count is 16384 (128^128).
      {
        tcI = uint2(dtId % dim.x, dtId/dim.x);
      }
      float rawDepth = loadGbufferDepth(tcI);
      if (rawDepth<=0)
        return;
      float2 tc = (tcI+0.5)/dim;
      float3 ndcCoord = float3(tc.xy*float2(2, -2) - float2(1, -1), rawDepth);
      float4 worldpos_prj = mul(float4(ndcCoord,1), globtm_inv);
      float3 worldPos = worldpos_prj.xyz / worldpos_prj.w;
      float3 toCamera = (worldPos-world_view_pos);

      if (has_physobj_in_cascade &&
          isInsideBbox(worldPos, ssgi_restricted_update_bbox_min, ssgi_restricted_update_bbox_max))
        return;

      //worldPos = world_view_pos + lerp(lerp(view_vecLT, view_vecRT, tc.x),
      //                            lerp(view_vecLB, view_vecRB, tc.x), tc.y)*linearize_z(rawDepth, zn_zfar);
      //float3 worldNormal; float smooth;
      //readPackedGbufferNormalSmoothness(tc, worldNormal, smooth);

      uint minCascade = 0, maxCascades = getSceneVoxelNumCascades();
      int3 baseCoord = 0;
      for (minCascade = 0; minCascade < maxCascades; ++minCascade)
      {
        baseCoord = sceneWorldPosToCoord(worldPos, minCascade);
        if (all(baseCoord >= 0 && baseCoord < VOXEL_RESOLUTION))
          break;
      }
      if (minCascade == maxCascades)
        return;
      uint cascade = minCascade;
      float rndPos = randPos(worldPos)+sin(ssgi_current_frame.x*0.1317);
      float randVal = rand(rndPos);
      //float chance = pow2(1./(maxCascades-cascade+1));
      float chance = 0.25;
      if (randVal < chance)
      {
        cascade = min(floor(lerp((float)minCascade, (float)maxCascades, pow16(randVal/chance))+0.5), maxCascades-1);
        baseCoord = sceneWorldPosToCoord(worldPos, cascade);
      }
      //voxels_alpha
      uint3 encodedVoxel;
      uint3 wrappedCoord = wrapSceneVoxelCoord(baseCoord, cascade);
      float3 voxelSize = getSceneVoxelSize(cascade);

      //lit voxels
      float3 next_color =0; float next_alpha =0;
      float w = linearize_z(rawDepth, zn_zfar.zw);
      int ret = get_voxel_scene_gbuffer_color(tc, w, worldPos, next_color, next_alpha, baseCoord, voxelSize, cascade);
      if (ret == 0)
        return;
      float current_alpha = getVoxelsAlphaDirect(wrappedCoord, cascade);
      float3 current_color = getVoxelsColor(wrappedCoord, cascade);
      if (next_alpha < 1)
        next_alpha = min(next_alpha, MAX_SCENE_VOXELS_ALPHA_TRANSPARENT);//to keep it transparent
      next_color *= next_alpha;//premultiplied alpha to allow light injection

      if (current_color.r == 0 || current_alpha < 0.01 || (next_alpha==1 && current_alpha < 1 && current_alpha > 0.99))//special case - data is not inited, we need temporal very fast
      {
        current_alpha = next_alpha;
        current_color = next_color;
      }

      float3 invVoxelSize = rcp(voxelSize);
      toCamera *= invVoxelSize;
      float expWAlpha = 0.125; float expW = (0.075 + 0.02 * lerpBrightnessValue(current_color, next_color))*saturate(dot(toCamera,toCamera)-invVoxelSize.x);
      float result_alpha = lerp(current_alpha, next_alpha, expWAlpha);
      FLATTEN
      if (next_alpha >= 1 || current_alpha >= 1)// do not ever make opaque volume transparent, but still update it's color
        result_alpha = 1;
      float3 result_color = lerp(current_color, next_color, expW);//limit maximum amount of light that can be added by translucent voxel
      result_color.r = max(result_color.r, MIN_REPRESENTABLE_SCENE_VALUE);//2^-15, enough to represent 10bit float 
      if (ret == 2)
      {
        result_alpha = 0;
        result_color = 0;
      }

      uint resultAlpha = uint(OLD_ALPHA_MAX_VALUE*result_alpha + 0.5);
      encodedVoxel.x = encode_scene_voxel_coord_bin(uint3(wrappedCoord), cascade, resultAlpha);
      encodedVoxel.y = encode_scene_temp_color(result_color);
      uint at;
      dispatchIndirectBuffer.InterlockedAdd(3 * 4, 1u, at);
      structuredBufferAt(gbuf_voxels, at) = encodedVoxel.xy;
    }
  }
  compile("cs_5_0", "mark_voxels_cs");
}

int color_reg_no = 0;
int alpha_reg_no = 1;

shader ssgi_temporal_scene_voxels_cs
{
  INIT_VOXELS(cs)
  USE_VOXELS(cs)
  SSGE_WRITE_VOXEL_DATA(cs, color_reg_no, alpha_reg_no)

  hlsl(cs) {
    StructuredBuffer<GBUF_VOXEL> temporal_voxels: register(t15);

    [numthreads(MARK_VOXELS_WARP_SIZE, 1, 1)]
    void mark_voxels_cs( uint dtId : SV_DispatchThreadID )
    {
      uint voxelNo = dtId;
      uint bin, cascade;
      uint3 coord = decode_scene_voxel_coord_safe(temporal_voxels[voxelNo].x, cascade, bin);
      float3 color = decode_scene_temp_color(temporal_voxels[voxelNo].y);
      float alpha = float(bin + 0.5) / OLD_ALPHA_MAX_VALUE;
      writeVoxelData(coord, cascade, color, alpha);
    }
  }
  compile("cs_5_0", "mark_voxels_cs");
}

shader ssgi_temporal_scene_voxels_create_cs {
  ENABLE_ASSERT(cs)
  hlsl(cs) {
    RWByteAddressBuffer dispatchIndirectBuffer : register( u0 );
    [numthreads( 1, 1, 1 )]
    void main()
    {
      storeBuffer(dispatchIndirectBuffer, 0 * 4, loadBuffer(dispatchIndirectBuffer, 3 * 4) / MARK_VOXELS_WARP_SIZE); // (numVoxels + MARK_VOXELS_WARP_SIZE-1)/MARK_VOXELS_WARP_SIZE;
      storeBuffer(dispatchIndirectBuffer, 1 * 4, 1);
      storeBuffer(dispatchIndirectBuffer, 2 * 4, 1);
      storeBuffer(dispatchIndirectBuffer, 3 * 4, 0);
    }
  }
  compile("cs_5_0", "main");
}


define_macro_if_not_defined INIT_VOXELS_SCENE_HELPERS(code)
  hlsl(code) {
    void get_voxel_scene_initial_color(float3 worldPos, float3 voxelSize, int cascade, inout float3 color, inout float alpha){}
  }
endmacro

shader ssgi_clear_scene_cs
{
  USE_VOXELS(cs)
  INIT_VOXELS_SCENE_HELPERS(cs)
  INIT_VOXELS(cs)
  ENABLE_ASSERT(cs)
  (cs) {
    scene_voxels_invalid_start@f4 = scene_voxels_invalid_start;
    scene_voxels_invalid_width@f4 = scene_voxels_invalid_width;
  }
  SSGE_WRITE_VOXEL_DATA(cs, color_reg_no, alpha_reg_no)

  hlsl(cs) {
    [numthreads(32, 1, 1)]
    void ssgi_clear_scene_cs( uint dtId : SV_DispatchThreadID )//
    {
      int3 startInvalid = int3(scene_voxels_invalid_start.xyz);
      uint4 invalidWidth = uint4(scene_voxels_invalid_width);
      if (dtId >= invalidWidth.w)
        return;
      int3 coord = (startInvalid.xzy + int3(dtId%invalidWidth.x, (dtId%(invalidWidth.x*invalidWidth.z))/invalidWidth.x, dtId/(invalidWidth.x*invalidWidth.z)));
      uint cascade = scene_voxels_invalid_start.w;
      float3 voxelSize = getSceneVoxelSize(cascade);

      float3 worldPos = voxelSize*(float3(coord.xzy)+0.5);

      uint3 wrappedCoord = (coord + (int3(VOXEL_RESOLUTION)/2))&uint3(VOXEL_RESOLUTION-1);
      float3 color = 0; float alpha =0;
      get_voxel_scene_initial_color(worldPos, voxelSize, cascade, color, alpha);
      writeVoxelData(wrappedCoord, cascade, color, alpha);
    }
  }
  compile("cs_5_0", "ssgi_clear_scene_cs");
}


buffer invalidBoxes;

shader ssgi_ivalidate_voxels_cs
{
  USE_VOXELS(cs)
  INIT_VOXELS_SCENE_HELPERS(cs)
  SSGE_SCENE_COMMON_INIT_WRITE(cs)
  SSGE_WRITE_VOXEL_DATA(cs, color_reg_no, alpha_reg_no)

  (cs) {
    invalidBoxes@buf = invalidBoxes hlsl {
      struct BBox4 {
        float4 bmin;
        float4 bmax;
      };
      StructuredBuffer<BBox4> invalidBoxes@buf;
    }
  }

  hlsl(cs) {
    #define WORK_GROUP_SIZE 64
    [numthreads(WORK_GROUP_SIZE, 1, 1)]
    void ssgi_invalidate_voxels_cs(uint id : SV_GroupIndex, uint3 gId: SV_GroupID)
    {
      const uint cascade = gId.y;
      BBox4 box = invalidBoxes[gId.x];
      float3 minPos = box.bmin.xyz;
      float3 maxPos = box.bmax.xyz;

      const float3 voxelSize = getSceneVoxelSize(cascade);
      const float3 sceneOrigin = getSceneVoxelOrigin(cascade);

      minPos = (minPos.xzy - sceneOrigin.xzy)/voxelSize.xzy;
      maxPos = (maxPos.xzy - sceneOrigin.xzy)/voxelSize.xzy;

      int3 coord = clamp(int3(floor(minPos)), int3(0, 0, 0), int3(VOXEL_RESOLUTION));
      int3 maxCoord = clamp(int3(ceil(maxPos)), int3(0, 0, 0), int3(VOXEL_RESOLUTION));
      uint3 voxelBox = uint3(maxCoord - coord);
      const uint voxelsCount = voxelBox.x * voxelBox.y * voxelBox.z;

      const uint workGroupSize = WORK_GROUP_SIZE;
      const float3 color = {0, 0, 0}; const float alpha = 0;
      const uint2 div = uint2(voxelBox.x, voxelBox.x * voxelBox.y);
      for(; id < voxelsCount; id += workGroupSize)
      {
        uint3 loc_offset = uint3(id, id, id);
        loc_offset.xy %= div;
        loc_offset.yz /= div;
        uint3 wrappedCoord = wrapSceneVoxelCoord(coord + loc_offset, cascade);
        writeVoxelData(wrappedCoord, cascade, color, alpha);
      }
    }
  }
  compile("cs_5_0", "ssgi_invalidate_voxels_cs");
}