include "sky_shader_global.sh"
include "viewVecVS.sh"
include "clouds_tiled_dist.sh"
include "clouds_close_layer_outside.sh"
include "skies_special_vision.sh"
include "distanceToClouds2.sh"
include "vr_reprojection.sh"
include "use_custom_fog_sky.sh"

float min_ground_offset;
texture clouds_color;
texture clouds_color_close;

texture clouds_target_depth_gbuf;
float4 clouds_target_depth_gbuf_transform = (1, 1, 0, 0);
texture clouds_depth_gbuf;
float4 clouds2_resolution;
int clouds_has_close_sequence = 1;
int clouds_apply_simple_ztest = 1;

shader clouds2_apply, clouds2_apply_has_empty, clouds2_apply_no_empty
{
  blend_src = 1; blend_dst = isa;
  cull_mode=none;
  z_write=false;
  //z_test=false;
  z_test=true; //fastest, but requires clouds to be far, than anything
  if (clouds_depth_gbuf == NULL)
  {
    hlsl {
      #define SIMPLE_APPLY 1
    }
  }
  USE_CLOUDS_DISTANCE(ps)
  USE_CLOUDS_DISTANCE_STUB(ps)
  USE_SPECIAL_VISION()
  SKY_HDR()

  INIT_BOUNDING_VIEW_REPROJECTION(ps)
  USE_BOUNDING_VIEW_REPROJECTION(ps)

  USE_CUSTOM_FOG_SKY(ps)

  VIEW_VEC_OPTIMIZED(ps)
  ENABLE_ASSERT(ps)

  (ps)
  {
    world_view_pos__clouds_start_alt@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, clouds_start_altitude2 * 1000.0);
  }
  hlsl(ps) {
    #define world_view_pos world_view_pos__clouds_start_alt.xyz
    #define clouds_start_alt world_view_pos__clouds_start_alt.w
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 viewVect : TEXCOORD0;
      float2 tc : TEXCOORD1;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()
  USE_AND_INIT_VIEW_VEC_VS()
  (vs) {
    encode_depth@f3 = (zn_zfar.x / (zn_zfar.y - zn_zfar.x), zn_zfar.y*zn_zfar.x / (zn_zfar.y - zn_zfar.x),
      max(40, clouds_start_altitude2 * 1000 - world_view_pos.y), 0);
    clouds_apply_simple_ztest@f1 = (clouds_apply_simple_ztest, 0, 0, 0);
  }

  hlsl(vs) {
    VsOutput apply_clouds_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float minCloudDist = encode_depth.z;
      float z = minCloudDist*encode_depth.x+encode_depth.y;
      #if SIMPLE_APPLY
      if (clouds_apply_simple_ztest > 0)
        { minCloudDist = 1; z = 0; }
      #endif
      float2 pos = getPostfxVertexPositionById(vertexId);
      output.pos = float4(pos.xy*minCloudDist, z, minCloudDist);
      output.tc = screen_to_texcoords(pos);
      output.viewVect = get_view_vec_by_vertex_id(vertexId);
      return output;
    }
  }

  (ps) {
    fullres_depth_gbuf@smp2d = clouds_target_depth_gbuf;
    fullres_depth_gbuf_transform@f4 = clouds_target_depth_gbuf_transform;
    clouds_depth_gbuf@smp2d = clouds_depth_gbuf;

    clouds_color@smp2d = clouds_color;
    clouds_color_close@smp2d = clouds_color_close;
    clouds2_far_res@f2 = (clouds2_resolution.xy,0,0);
    clouds2_far_res_minus_1@f2 = (clouds2_resolution.x - 1, clouds2_resolution.y - 1);
    clouds2_close_res@f2 = (clouds2_resolution.zw,0,0);
    clouds_has_close_sequence@f1 = (clouds_has_close_sequence);
  }
  INIT_ZNZFAR()
  DISTANCE_TO_CLOUDS2(ps)
  CLOSE_LAYER_EARLY_EXIT(ps)
  if (is_gather4_supported == supported || shader != clouds2_apply)
  {
    hlsl {
      #define HAS_GATHER 1
    }
  }

  if (shader == clouds2_apply_no_empty)
  {
    hlsl {
      #define HAS_EMPTY_TILES 0
    }
  } else if (shader == clouds2_apply_has_empty || shader == clouds2_apply)
  {
    hlsl {
      #define HAS_EMPTY_TILES 1
    }
  }

  hlsl(ps) {
    #include "daCloudsTonemap.hlsl"
    #include "cloud_settings.hlsli"
    float4 getZWeights(float4 lowResLinearZ, float hiResLinearZ)
    {
      return rcp( 0.00001 + abs( lowResLinearZ - hiResLinearZ ) );
    }
    float4 linearize_z4(float4 raw4, float2 decode_depth)
    {
      return rcp(decode_depth.xxxx + decode_depth.y * raw4);
    }

    #include <tex2d_bicubic.hlsl>
    half4 getBicubicClose( float2 p )
    {
##if clouds_use_fullres == yes
      return tex2Dlod(clouds_color_close, float4(p,0,0));
##else
      return tex2D_bicubic_lod(clouds_color_close, clouds_color_close_samplerstate, p, clouds2_close_res, 0);
##endif
    }

    half4 bilateral_get(float3 viewVec, float2 texcoord, float2 depth_texcoord, float linearDepth, float rawDepth)
    {
      float3 worldPos = viewVec*linearDepth + world_view_pos;
      bool isFullRayBelowClouds = worldPos.y < clouds_start_alt && world_view_pos.y < clouds_start_alt;
      if (isFullRayBelowClouds && rawDepth != 0)
        return half4(0,0,0,0);

##if clouds_use_fullres == yes
      return tex2Dlod(clouds_color, float4(texcoord,0,0));
##endif

      #define GET_LOWRES_COORD(ofs, tc)\
        float2 lowResCoords = tc*clouds2_far_res - ofs;\
        int4 lowResCoordsI;\
        lowResCoordsI.xy = int2(lowResCoords);\
        lowResCoordsI.zw = min(lowResCoordsI.xy+int2(1,1), (int2)clouds2_far_res_minus_1);

      #if HAS_GATHER
        float4 lowResRawDepth = clouds_depth_gbuf.GatherRed(clouds_depth_gbuf_samplerstate, depth_texcoord).wzxy;
      #else
        GET_LOWRES_COORD(0.5, depth_texcoord)
        float4 lowResRawDepth;
        lowResRawDepth.x = clouds_depth_gbuf[lowResCoordsI.xy].x;
        lowResRawDepth.y = clouds_depth_gbuf[lowResCoordsI.zy].x;
        lowResRawDepth.z = clouds_depth_gbuf[lowResCoordsI.xw].x;
        lowResRawDepth.w = clouds_depth_gbuf[lowResCoordsI.zw].x;
      #endif
      float4 linearLowResDepth = linearize_z4(lowResRawDepth, zn_zfar.zw);
      float4 maxDiff4 = abs(linearLowResDepth-linearDepth);
      float maxDiff = max(max(maxDiff4.x, maxDiff4.y), max(maxDiff4.z, maxDiff4.w));
      float minDiff = min(min(maxDiff4.x, maxDiff4.y), min(maxDiff4.z, maxDiff4.w));
      BRANCH
      if (maxDiff<linearDepth*0.05 || minDiff>linearDepth*0.5)//if difference is within 5%  - just use bilinear
        return tex2Dlod(clouds_color, float4(texcoord,0,0));

      #if HAS_GATHER
        GET_LOWRES_COORD(0.499, texcoord)
      #endif
      float4 linearLowResWeights = getZWeights(linearLowResDepth, linearDepth);
      linearLowResWeights*=linearLowResWeights;
      linearLowResWeights /= dot(linearLowResWeights, 1);

      return clouds_color[lowResCoordsI.xy]*linearLowResWeights.x+
             clouds_color[lowResCoordsI.zy]*linearLowResWeights.y+
             clouds_color[lowResCoordsI.xw]*linearLowResWeights.z+
             clouds_color[lowResCoordsI.zw]*linearLowResWeights.w;
    }

    half4 apply_clouds_ps_main(VsOutput input, float4 screenpos, out float raw_depth)
    {
      float2 depth_texcoord = input.tc;
      raw_depth = tex2Dlod(fullres_depth_gbuf, float4(input.tc.xy * fullres_depth_gbuf_transform.xy + fullres_depth_gbuf_transform.zw,0,0)).x;


      // On DX10 we cannot use depth as a target and as a shader resouce at the same time. Do the depth test in the shader
    ##if !hardware.fsh_5_0
      if (screenpos.z < raw_depth)
        return 0.0f;
    ##endif


      float2 texcoord = input.tc;
##if use_bounding_vr_reprojection == on
      texcoord = vr_bounding_view_reproject_tc(texcoord,0);
      depth_texcoord = texcoord;
##endif
      #if SIMPLE_APPLY
        half4 distPlane = 0;
        if (HAS_EMPTY_TILES==0 || !tile_is_empty(uint2(texcoord.xy*clouds2_far_res)))
          distPlane = tex2Dlod(clouds_color, float4(texcoord,0,0));
        return half4(TAA_BRIGHTNESS_SCALE*distPlane.rgb, distPlane.a);
      #else
        //we can exit early doing bilteral sample apply, if (for bilinear):
        // +tile is empty (only apply to 'far plane' (of if there is no 'close plane')
        // *all 4 texels of high res depth are zfar. which is the same, as sampling close_depth_tex from required mip and check if it's z-far.
        // *closest possible distance to cloud is still bigger than hires sample depth (happens often on a ground)
        // *all 4 lowres texels of lowres clouds are 0 (no clouds there)

        //todo: check tile and exit immediately if close_layer_should_early_exit(), otherwise just apply close layer.

        float3 view = normalize(input.viewVect);
        #ifndef CHECK_DIST_TO_CLOUDS
        #define CHECK_DIST_TO_CLOUDS 0
        #endif
        #if CHECK_DIST_TO_CLOUDS
        //can happen only when we are above/below clouds layer
        // tht is so rare, that doesn't make sense to optimize
        float distToClouds = 0;
        float dist1; distance_to_clouds(-view, distToClouds, dist1);
        distToClouds *= 1000;
        #endif

        float linearDepth = linearize_z(raw_depth, zn_zfar.zw);
        float linearDist = linearDepth*length(input.viewVect);
        half4 distPlane = 0;
        half4 closePlane = 0;
        BRANCH
        if (!close_layer_should_early_exit())
          closePlane = getBicubicClose(texcoord);
        float closeSequenceEndDist = clouds_has_close_sequence ? closeSequenceStepSize*(closeSequenceSteps-4) : 0;
        #if CHECK_DIST_TO_CLOUDS
        //can happen only when we are above/below clouds layer
        // tht is so rare, that doesn't make sense to optimize
        if (distToClouds > linearDist && raw_depth != 0)
        {
        } else
        #endif
        {
          float3 viewVec = getViewVecOptimized(input.tc);
          if (linearDist > closeSequenceEndDist && (HAS_EMPTY_TILES==0 || !tile_is_empty(uint2(texcoord.xy*clouds2_far_res))))
            distPlane = bilateral_get(viewVec, texcoord, depth_texcoord, linearDepth, raw_depth);
        }
        return half4(TAA_BRIGHTNESS_SCALE*(distPlane.rgb*(1-closePlane.a) + closePlane.rgb), 1-(1-closePlane.a)*(1-distPlane.a));
      #endif
    }

    half4 apply_clouds_ps(VsOutput input HW_USE_SCREEN_POS) : SV_Target
    {
      float4 screenpos = GET_SCREEN_POS(input.pos);
      float rawDepth;
      half4 result = apply_clouds_ps_main(input, screenpos, rawDepth);

      // fog is applied here, in clouds apply instead of sky rendering, as clouds are drawn later, but we don't want +1 pass for fog apply
      // we assume volfog is always in front of clouds (we already assume that for every surface in the depth buffer anyway)
      // otherwise we would apply fog multiple times in a certain range (from encode_depth.z)
      if (rawDepth == 0)
      {
        result.a = 1 - result.a; // transmittance is the inverse of blending alpha
        float2 jitteredVolfogTc = get_volfog_dithered_screen_tc(screenpos.xy, input.tc.xy);
        apply_sky_custom_fog(result, input.tc.xy, jitteredVolfogTc);
        result.a = 1 - result.a;
      }

      applySpecialVision(result);
      result.rgb = pack_hdr(result.rgb);
      return result;
    }
  }

  compile("target_vs", "apply_clouds_vs");
  if (is_gather4_supported == supported || shader != clouds2_apply)
  {
    compile("ps_4_1", "apply_clouds_ps");
  } else
  {
    compile("target_ps", "apply_clouds_ps");
  }
}
float4 clouds2_dispatch_groups;
shader clouds_create_indirect
{
  CLOSE_LAYER_EARLY_EXIT(cs)

  (cs) {
    tiles_threshold@f1 = (clouds_tiled_res.x*clouds_tiled_res.y*0.9, clouds2_dispatch_groups.x,clouds2_dispatch_groups.y,0);//90% of tiles should be non empty so we ignore non-empty tiles completely
    clouds2_dispatch_groups@f4 = clouds2_dispatch_groups;
  }

  hlsl(cs) {
    RWByteAddressBuffer indirect_buffer:register(u0);
    #include <clouds2/cloud_settings.hlsli>

    [numthreads(CLOUDS_APPLY_COUNT, 1, 1)]
    void cs_main(uint flatIdx : SV_GroupIndex)
    {
      uint2 groups = uint2(asuint(clouds2_dispatch_groups.x), asuint(clouds2_dispatch_groups.y));
      bool noEmpty = clouds_non_empty_tile_count_ge(tiles_threshold.x);
      uint targetId = noEmpty ? CLOUDS_NO_EMPTY : CLOUDS_HAS_EMPTY;
      FLATTEN
      if (flatIdx%CLOUDS_APPLY_COUNT_PS == CLOUDS_HAS_CLOSE_LAYER)
      {
        groups = uint2(asuint(clouds2_dispatch_groups.z), asuint(clouds2_dispatch_groups.w));
        targetId = close_layer_should_early_exit() ? 10000 : flatIdx%CLOUDS_APPLY_COUNT_PS;
      }
      uint3 write_first = flatIdx>=CLOUDS_APPLY_COUNT_PS ? uint3(groups, 1) : uint3(3,1,0);
      bool shouldWrite = (flatIdx%CLOUDS_APPLY_COUNT_PS) == targetId;
      storeBuffer(indirect_buffer, (flatIdx * 4 + 0) * 4, shouldWrite ? write_first.x : 0);
      storeBuffer(indirect_buffer, (flatIdx * 4 + 1) * 4, shouldWrite ? write_first.y : 0);
      storeBuffer(indirect_buffer, (flatIdx * 4 + 2) * 4, shouldWrite ? write_first.z : 0);
      storeBuffer(indirect_buffer, (flatIdx * 4 + 3) * 4, 0);
    }
  }
  compile("cs_5_0", "cs_main");
}
