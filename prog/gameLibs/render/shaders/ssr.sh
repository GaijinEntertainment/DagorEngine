include "shader_global.sh"
include "hero_matrix_inc.sh"
include "reprojected_motion_vectors.sh"
include "gbuffer.sh"
include "viewVecVS.sh"
include "monteCarlo.sh"
include "ssr_common.sh"
include "vsm.sh"
include "frustum.sh"
include "alternate_reflections.sh"
include "ssr_inc.sh"
include "separate_depth_mips.sh"
include "force_ignore_history.sh"

int ssr_quality = 1;
interval ssr_quality : low<1, medium<2, high<3, ultra;

int ssr_ffx = 0;
interval ssr_ffx : no<1, yes;

texture downsampled_checkerboard_depth_tex;

texture ssr_target;
texture downsampled_normals;
texture ssr_prev_target;
texture prev_downsampled_far_depth_tex;

float4 ssr_filter0;
float4 ssr_filter1;
float ssr_filter2;
float ssr_clamping_gamma = 1.0;
float ssr_neighborhood_bias_tight = 0.0;
float ssr_neighborhood_bias_wide = 0.05;
float ssr_neighborhood_velocity_difference_scale = 100;

texture ssr_strength;
texture vtr_target;
float4 vtr_target_size=(1280 / 4, 720 / 4, 0, 0);

float4 analytic_light_sphere_pos_r=(0,0,0,0);
float4 analytic_light_sphere_color=(1,0,0,1);

macro INIT_TEXTURES(code)
  if (ssr_resolution == halfres) {
    if (downsampled_checkerboard_depth_tex != NULL) {
      (code) { depth_tex@smp2d = downsampled_checkerboard_depth_tex; }
    } else {
      (code) { depth_tex@smp2d = downsampled_far_depth_tex; }
    }

    if (separate_depth_mips == no) {
      (code) {
        far_depth_tex@smp2d = downsampled_far_depth_tex;
        normals_tex@smp2d = downsampled_normals;
      }
    } else {
      INIT_SEPARATE_DEPTH_MIPS(code)
      USE_SEPARATE_DEPTH_MIPS(code)
      (code) {
        normals_tex@smp2d = downsampled_normals;
      }
    }
  } else {
    INIT_READ_GBUFFER_BASE(code)
    INIT_READ_DEPTH_GBUFFER_BASE(code)
    hlsl(code) {
      #define depth_tex depth_gbuf_read
      #define depth_tex_samplerstate depth_gbuf_read_samplerstate
      #define normals_tex normal_gbuf_read
      #define normals_tex_samplerstate normal_gbuf_read_samplerstate
    }
  }
endmacro

macro SSR_COMMON(code)
  if (ssr_ffx == yes && ssr_quality != low) { dont_render; }

  ENABLE_ASSERT(code)
  SETUP_SSR(code)
  VIEW_VEC_OPTIMIZED(code)
  INIT_HERO_MATRIX(code)
  USE_HERO_MATRIX(code)
  INIT_REPROJECTED_MOTION_VECTORS(code)
  USE_REPROJECTED_MOTION_VECTORS(code)
  INIT_TEXTURES(code)
  USE_IGNORE_HISTORY(code)

  (code) {
    world_view_pos@f4 = world_view_pos;
    ssr_world_view_pos@f4 = ssr_world_view_pos;
    prev_frame_tex@smp2d = prev_frame_tex;
    SSRParams@f4 = (SSRParams.x, SSRParams.y, ssr_frameNo.z, ssr_frameNo.y);
    ssr_target_size@f4 = ssr_target_size;
    ssr_prev_target@smp2d = ssr_prev_target;
    prev_downsampled_far_depth_tex@smp2d = prev_downsampled_far_depth_tex;

    prev_globtm_no_ofs_psf@f44 = { prev_globtm_no_ofs_psf_0, prev_globtm_no_ofs_psf_1, prev_globtm_no_ofs_psf_2, prev_globtm_no_ofs_psf_3 };
    globtm_no_ofs_psf@f44 = { globtm_no_ofs_psf_0, globtm_no_ofs_psf_1, globtm_no_ofs_psf_2, globtm_no_ofs_psf_3 };

    water_level@f1 = water_level;

    encode_depth@f4 = (-zn_zfar.x/(zn_zfar.y-zn_zfar.x), zn_zfar.x * zn_zfar.y/(zn_zfar.y-zn_zfar.x),
                      zn_zfar.x/(zn_zfar.x * zn_zfar.y), (zn_zfar.y-zn_zfar.x)/(zn_zfar.x * zn_zfar.y));

    analytic_light_sphere_pos_r@f4 = analytic_light_sphere_pos_r;
    analytic_light_sphere_color@f4 = analytic_light_sphere_color;
  }

  if (ssr_ffx == yes)
  {
    (code)
    {
      downsampled_close_depth_tex@tex = downsampled_close_depth_tex hlsl { Texture2D<float>downsampled_close_depth_tex@tex; }
      downsampled_depth_mip_count@f1 = (downsampled_depth_mip_count);
      lowres_rt_params@f2 = (lowres_rt_params.x, lowres_rt_params.y, 0, 0); //Far and Close depth are the same size
    }
  }

  hlsl(code) {
    ##if ssr_quality == medium
      #define SSR_QUALITY 2
    ##elif ssr_quality == high
      #define SSR_QUALITY 3
    ##elif ssr_quality == ultra
      #define SSR_QUALITY 4
    ##else
      #define SSR_QUALITY 1
    ##endif

    ##if ssr_ffx == yes
      #define SSR_FFX
      #define FFX_SSSR_INVERTED_DEPTH_RANGE
      #include "../../../3rdPartyLibs/ssr/ffx_sssr.h"
    ##endif

    #define PREV_HERO_SPHERE 1

    ##if ssr_resolution == halfres
      ##if separate_depth_mips == no
        #define ssr_depth far_depth_tex
      ##else
        #define SSR_USE_SEPARATE_DEPTH_MIPS
      ##endif
    ##else
      #define ssr_depth depth_tex
    ##endif

    #ifndef MOTION_VECTORS_TEXTURE
      #define MOTION_VECTORS_TEXTURE motion_gbuf
    #endif
    #ifndef CHECK_VALID_MOTION_VECTOR
      #define CHECK_VALID_MOTION_VECTOR(a) (a.x != 0)
    #endif

    void unpack_material(float2 texcoord, out half3 normal, out half linear_roughness, out float smoothness)
    {
      half4 normal_smoothness = tex2Dlod(normals_tex, float4(texcoord, 0, 0));
      normal = normalize(normal_smoothness.xyz * 2 - 1);
##if ssr_resolution == halfres
      smoothness = normal_smoothness.w;
##else
      smoothness = tex2Dlod(material_gbuf_read, float4(texcoord, 0, 0)).x;
##endif
      linear_roughness = linearSmoothnessToLinearRoughness(smoothness);
    }
  }
endmacro

float4 ssr_tile_jitter = (0, 0, 0, 0);

shader tile_ssr_compute
{
  SSR_COMMON(cs)
  SSR_CALCULATE(cs)
  INIT_ZNZFAR_STAGE(cs)

  (cs) {
     ssr_tile_jitter@f2 = (ssr_tile_jitter);
  }

  hlsl(cs) {
    #include <BRDF.hlsl>

    #define TILE_BITS 12
    #define TILE_MASK ((1U<<TILE_BITS)-1)
    #define TILE_SHIFT 3
    #define TILE_SIZE 8

    RWTexture2D<float4> Result:register(u0);

    #define groupthreads (TILE_SIZE*TILE_SIZE)

    groupshared float4 ColorTarg[groupthreads];
    groupshared float3 VecTarg[groupthreads];
    groupshared float depthTarg[groupthreads];

    /////////////////////////////////////
    // Resolve function
    // GTid - groupthread ID
    // ray - camera-to-pixel vector
    // normal - view space normal
    // PerfectReflection
    // pixel - view space coords of pixel
    // alpha2, NdV - BRDF supporting values
    // uints2 x_samp, y_samp - size of mini-tile
    /////////////////////////////////////
    float4 ResolvePixels(float4 val, uint2 GTid, float3 pointToEyeDir, float3 normal, float3 offsetPoint, float linear_roughness, uint samp, float currentDepth)
    {
      float4 Out = 0.01*val;

      float TotalWeight = 0.01;
      //float3 TotalReflection = 0;

      // Mid sample for given pixel position and size of mini-tile
      int midi = GTid.x;//clamp(GTid.x, samp, TILE_SIZE - 1 - samp);
      int midj = GTid.y;//clamp(GTid.y, samp, TILE_SIZE - 1 - samp);

      int i = max(midi - samp, 0);
      int maxi = min(midi + samp, TILE_SIZE - 1);
      int maxj = min(midj + samp, TILE_SIZE - 1);
      while (i <= maxi)
      {
        int j = max(midj - samp, 0);
        while (j <= maxj)
        {
          uint groupIndex = j*TILE_SIZE + i;
          // calculate reflection with compensation for neighboring pixels
          float3 CurrentReflection = normalize(VecTarg[groupIndex].xyz + offsetPoint);

          // supporting values
          float3 HalfVec = normalize(CurrentReflection.xyz + pointToEyeDir);
          float NoH = saturate(dot(normal, HalfVec));
          float NoL = saturate(dot(normal, CurrentReflection));

          float D_GGX = NoL*BRDF_distribution( pow2(linear_roughness), NoH );

          float4 sampleColor = ColorTarg[groupIndex];
          float sampleWeight = max(0, D_GGX * (1.2 + 2*linear_roughness - abs(i-midi))*(1.2 + 2*linear_roughness - abs(j-midj)));
          //float sampleWeight = D_GGX;

          sampleWeight *= saturate(2 - 50*abs(currentDepth - depthTarg[groupIndex])/currentDepth);

          TotalWeight += sampleWeight; // Integration of microfacet distribution (PDF)

          Out += sampleWeight*sampleColor; // Integration of BRDF weighted color
          j++;
        }
        i++;
      }
      // divide by sum NDF
      Out /= TotalWeight;

      return Out;
    }

    float4 normal_aware_blur(uint2 GTid, int GI, float4 Out, float depth, float depthThresholdInv, float linear_roughness)
    {
      float wMul = 1 + (GTid.x == 3 || GTid.x == 4) + (GTid.y == 3 || GTid.y == 4);

      // Mid sample for given pixel position and size of mini-tile
      depth*=depthThresholdInv;//basically it is constant!
      #define ADD_BLUR(cindex, weight)\
      {\
        int index=cindex;\
        float w = wMul*weight*saturate(1-abs(depthTarg[index].x*depthThresholdInv-depth));\
        Out += w * ColorTarg[index];\
        TotalWeight += w;\
      }

      float TotalWeight = 1;//is always 1
      const float centerWeight = 1.5;
      float sideW = 1/centerWeight, cornerW = 0.7/centerWeight;
      if (GTid.x>0)
      {
        ADD_BLUR(GI-1, sideW);
        if (GTid.y>0)
          ADD_BLUR(GI-1-TILE_SIZE, cornerW);
        if (GTid.y<TILE_SIZE-1)
          ADD_BLUR(GI-1+TILE_SIZE, cornerW);
      }
      if (GTid.x<TILE_SIZE-1)
      {
        ADD_BLUR(GI+1, sideW);
        if (GTid.y>0)
          ADD_BLUR(GI+1-TILE_SIZE, cornerW);
        if (GTid.y<TILE_SIZE-1)
          ADD_BLUR(GI+1+TILE_SIZE, cornerW);
      }

      if (GTid.y>0)
        ADD_BLUR(GI-TILE_SIZE, sideW);
      if (GTid.y<TILE_SIZE-1)
        ADD_BLUR(GI+TILE_SIZE, sideW);
      // divide by
      TotalWeight = max(TotalWeight, 0.0001);

      return Out/TotalWeight;
    }

    float3 accurateLinearToSRGBPos (in float3 linearCol )
    {
      float3 sRGBLo = linearCol * 12.92;
      float3 sRGBHi = ( pow( linearCol, 1.0/2.4) * 1.055) - 0.055;
      float3 sRGB = ( linearCol <= 0.0031308) ? sRGBLo : sRGBHi ;
      return sRGB ;
    }

    #define BLOCK_SIZE_W 8
    #define BLOCK_SIZE_H 8
    [numthreads(BLOCK_SIZE_W,BLOCK_SIZE_H,1)]
    void tile_ssr_cs( uint3 Groupid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
    {
      uint frameNo = uint(SSRParams.z);
      uint2 ssr_dispatch = ssr_target_size.xy + ((frameNo&1)?(TILE_SIZE/2): 0).xx;//const

    /*uint tileInfo = ssrTiles[Groupid.x];
      uint2 Gid = uint2(tileInfo, (tileInfo>>TILE_BITS))&TILE_MASK;
      uint traceCount = tileInfo>>(2*TILE_BITS);//currently not used
      uint2 screenCoord = Gid*(TILE_SIZE/2) + GTid.xy; */
      uint2 screenCoord = DTid.xy + uint2(0.5*BLOCK_SIZE_W*ssr_tile_jitter.x, 0.5*BLOCK_SIZE_H*ssr_tile_jitter.y);
      bool invalidPixel = any(screenCoord >= ssr_dispatch.xy);
      uint2 screenCoordOfs = screenCoord%uint2(ssr_target_size.xy);
      screenCoord = invalidPixel ? screenCoord : screenCoordOfs;

      float2 screenCoordCenter = screenCoord + float2(0.5,0.5);

      float2 curViewTc = saturate(screenCoordCenter*ssr_target_size.zw);
      float3 viewVect = getViewVecOptimized(curViewTc);

      half3 normal;
      half linear_roughness;
      float smoothness;
      unpack_material(curViewTc, normal, linear_roughness, smoothness);

      float rawDepth = tex2Dlod(depth_tex, float4(curViewTc,0,0)).x;

      float w = linearize_z(rawDepth, zn_zfar.zw);

      bool NEEDSSR = (linear_roughness < 0.7) && (w < 0.5*zn_zfar.y);

      float3 originToPoint = viewVect * w;
      float3 worldPos = world_view_pos.xyz + originToPoint;
      float3 realWorldPos = ssr_world_view_pos.xyz + originToPoint;
      uint2 pixelPos = screenCoord;
      float3 N = normal;
      float linearDepth = w;

      float4 newFrame = 0;
      float4 prevFrame = 0;
      float3 capturePoint = 0;
      float4 result = 0;
      float reprojectionWeight = 0;
      float2 oldUv = 0;

      #if SSR_TRACEWATER == 1
        if (underwater_params.w * (realWorldPos.y - water_level) < 0)
          NEEDSSR = false;
      #endif

      BRANCH
      if (NEEDSSR)
      {
        uint frameRandom = uint(SSRParams.w);
        uint2 random = ((GTid.xy + uint2(frameRandom%4, (frameRandom/4)%4)));
        float stepOfs = interleavedGradientNoiseFramed(pixelPos.xy, uint(SSRParams.z)) - 0.25 + 1+ linear_roughness;

        float3 perfectR = reflect( originToPoint, N );

        #define NUM_TEMPORAL_RAYS 16
        float2 E = hammersley( ((random.x&3) + 3*(random.y&3))&(NUM_TEMPORAL_RAYS-1), NUM_TEMPORAL_RAYS, random );

        float roughnessHack = linear_roughness;
        float3 H = tangent_to_world( importance_sample_GGX_NDF( E, roughnessHack ).xyz, N );
        float3 R = originToPoint- 2*dot( originToPoint, H)*H;

        R-=2*N*min(0, dot(R, N));

        #if SSR_TRACEWATER == 1
          float4 hit_uv_z_fade = hierarchRayMarch(float3(curViewTc, rawDepth), R, linear_roughness, w, originToPoint, stepOfs, globtm_no_ofs_psf, water_level-realWorldPos.y+worldPos.y);
        #else
          float4 hit_uv_z_fade = hierarchRayMarch(float3(curViewTc, rawDepth), R, linear_roughness, w, originToPoint, stepOfs, globtm_no_ofs_psf, 0);
        #endif

        // if there was a hit
        BRANCH if( hit_uv_z_fade.w > 0 )
        {
          hit_uv_z_fade.z = ssr_linearize_z(hit_uv_z_fade.z);
          newFrame = sample_vignetted_color( hit_uv_z_fade.xyz, linear_roughness, hit_uv_z_fade.z, originToPoint, N, realWorldPos)*SSRParams.r;
          float3 capViewVect = getViewVecOptimized(hit_uv_z_fade.xy);
          capturePoint = capViewVect * linearize_z(hit_uv_z_fade.z, zn_zfar.zw);
        }
      }
      //Resolve
      VecTarg[GI] = capturePoint;
      depthTarg[GI] = w;
      ColorTarg[GI] = newFrame;
      GroupMemoryBarrierWithGroupSync();

      BRANCH
      if (NEEDSSR)
      {
        uint res_Params = clamp(1+(1-smoothness)*2, 1, 3); // override
        res_Params = 1;
        float3 offsetPoint = -originToPoint;//  + 0.02*perfectR.xyz;
        float3 viewDir = normalize(viewVect);
        newFrame = ResolvePixels(newFrame, GTid.xy, -viewDir, normal, offsetPoint, linear_roughness, res_Params, w);
      }

      /////////////////////////////////////////////////
      // Temporal reprojection
      /////////////////////////////////////////////////
      BRANCH
      if (NEEDSSR)
      {
        float3 prevViewVec = originToPoint;
        float4 reprojectedWorldPos = reprojected_world_view_pos;
        float weight = 0.9 + linear_roughness* 0.07; //rougher surfaces should be more relaxed, to fight noise
        #if SSR_MOTIONREPROJ == 1
          float2 motion = tex2Dlod(MOTION_VECTORS_TEXTURE, float4(curViewTc,0,0)).rg;
          BRANCH
          if (CHECK_VALID_MOTION_VECTOR(motion)) // if we have  motion vectors
          {
            oldUv = curViewTc + decode_motion_vector(motion);

            ##if prev_downsampled_far_depth_tex != NULL
              float currentLinearDepth = w;
              float prevLinearDepth = linearize_z(tex2Dlod(prev_downsampled_far_depth_tex, float4(oldUv, 0, 0)).r, zn_zfar.zw);
              const float disocclusionDepthScale = 1.0f;
              float disocclusionWeightAtten = saturate(1.0 - disocclusionDepthScale * (currentLinearDepth - prevLinearDepth));
              weight *= disocclusionWeightAtten;
            ##endif
          }
          else
        #endif
          {
            #if SSR_MOTIONREPROJ != 1
              apply_hero_matrix(prevViewVec);
            #endif

            float4 prevClip = mul(float4(prevViewVec, 0), prev_globtm_no_ofs_psf) + reprojectedWorldPos;
            float3 prevScreen = prevClip.w > 0 ? prevClip.xyz*rcp(prevClip.w) : float3(2,2,0);
            bool offscreen = max(abs(prevScreen.x), abs(prevScreen.y)) >= 1.0f;
            FLATTEN if (offscreen)
              weight = 0;

            const float invalidateScale = 3;
            oldUv = prevScreen.xy*float2(0.5,-0.5) + float2(0.5,0.5);
            weight *= saturate(1 - dot(invalidateScale.xx, abs(curViewTc - oldUv.xy)));

            ##if prev_downsampled_far_depth_tex != NULL
              float prevRawDepth = tex2Dlod(prev_downsampled_far_depth_tex, float4(oldUv, 0, 0)).x;
              float4 prev_zn_zfar = zn_zfar;//check if it is correct assumption
              float prevW = linearize_z(prevRawDepth, prev_zn_zfar.zw);

              float moveScale=0.1+max(25*rcp(w), 0.5);
              // hack for flat surfaces oriented parallel to viewvect
              moveScale*=0.3+0.7*abs(dot(normal, normalize(viewVect)));
              float move = saturate(prevClip.w - prevW); // don't use abs(), depth difference only concerns us in disocclusion direction
              #if SSR_TRACEWATER == 1
                // prev_downsampled_far_depth_tex contain water, so underwater reprojection fails
                weight *= underwater_params.w*(water_level-realWorldPos.y) < 0 ? saturate(1 - move * moveScale) : 1;
              #else
                weight *= saturate(1 - move * moveScale);
              #endif
            ##endif
          }

        reprojectionWeight = weight;
      }

      prevFrame = tex2Dlod(ssr_prev_target, float4(oldUv, 0,0));
      newFrame = force_ignore_history == 0 ? lerp(newFrame, prevFrame, reprojectionWeight) : newFrame;

      //depth aware blur

      ColorTarg[GI] = newFrame;

      GroupMemoryBarrierWithGroupSync();

      BRANCH
      if (NEEDSSR)
        result = normal_aware_blur(GTid.xy, GI, newFrame, w, 0.20, linear_roughness);

      Result[screenCoord] = result;
    }
  }
  compile("cs_5_0", "tile_ssr_cs");
}

shader ssr
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  SSR_COMMON(ps)
  INIT_ZNZFAR()
  SSR_CALCULATE(ps)
  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  hlsl(ps) {
    half4 ssr_ps(VsOutput input) : SV_Target
    {
      half3 normal;
      half linear_roughness;
      float smoothness;
      unpack_material(input.texcoord, normal, linear_roughness, smoothness);

      // Early out
      float roughnessFade = saturate(linear_roughness * SSRParams.y + 2);
      BRANCH
      if (roughnessFade == 0)
        return 0;

      float rawDepth = tex2Dlod(depth_tex, float4(input.texcoord, 0, 0)).x;

      BRANCH
      if (rawDepth <= 0 || rawDepth >= 1)
        return 0;

      float w = linearize_z(rawDepth, zn_zfar.zw);
      float3 cameraToPoint = input.viewVect.xyz * w;
      float3 worldPos = world_view_pos.xyz + cameraToPoint.xyz;
      float3 realWorldPos = ssr_world_view_pos.xyz + cameraToPoint.xyz;
      float worldDist=length(cameraToPoint.xyz);

      uint2 pixelPos = uint2(input.texcoord.xy * ssr_target_size.xy);


      float4 reprojectedWorldPos = reprojected_world_view_pos;
      float3 prevWorldPos = worldPos;
      float3 prevViewVec = cameraToPoint;

      // temporal reprojection first
      //check if bbox of weapon
      float weight = 0.9 + linear_roughness* 0.03; //rougher surfaces should be more relaxed, to fight noise
      float2 oldUv = 0;
      #if SSR_MOTIONREPROJ == 1
        float2 motion = tex2Dlod(MOTION_VECTORS_TEXTURE, float4(input.texcoord,0,0)).rg;
        BRANCH
        if (CHECK_VALID_MOTION_VECTOR(motion)) // if we have  motion vectors
        {
          oldUv = input.texcoord + decode_motion_vector(motion);

          ##if prev_downsampled_far_depth_tex != NULL
            float currentLinearDepth = w;
            float prevLinearDepth = linearize_z(tex2Dlod(prev_downsampled_far_depth_tex, float4(oldUv, 0, 0)).r, zn_zfar.zw);
            const float disocclusionDepthScale = 1.0f;
            float disocclusionWeightAtten = saturate(1.0 - disocclusionDepthScale * (currentLinearDepth - prevLinearDepth));
            weight *= disocclusionWeightAtten;
          ##endif
        }
        else
      #endif
        {
          #if SSR_MOTIONREPROJ != 1
            apply_hero_matrix(prevViewVec);
          #endif

          float4 prevClip = mul(float4(prevViewVec, 0), prev_globtm_no_ofs_psf) + reprojectedWorldPos;
          float3 prevScreen = prevClip.w > 0 ? prevClip.xyz*rcp(prevClip.w) : float3(2,2,0);
          bool offscreen = max(abs(prevScreen.x), abs(prevScreen.y)) >= 1.0f;
          FLATTEN if (offscreen)
            weight = 0;

          const float invalidateScale = 3;
          oldUv = prevScreen.xy*float2(0.5,-0.5) + float2(0.5,0.5);
          weight *= saturate(1 - dot(invalidateScale.xx, abs(input.texcoord.xy - oldUv.xy)));

          ##if prev_downsampled_far_depth_tex != NULL
            float prevRawDepth = tex2Dlod(prev_downsampled_far_depth_tex, float4(oldUv, 0, 0)).x;
            float4 prev_zn_zfar = zn_zfar;//check if it is correct assumption
            float prevW = linearize_z(prevRawDepth, prev_zn_zfar.zw);

            float moveScale=0.1+max(25*rcp(w), 0.5);
            // hack for flat surfaces oriented parallel to viewvect
            moveScale*=0.3+0.7*abs(dot(normal, normalize(input.viewVect)));
            float move = saturate(prevClip.w - prevW); // don't use abs(), depth difference only concerns us in disocclusion direction
            #if SSR_TRACEWATER == 1
              // prev_downsampled_far_depth_tex contain water, so underwater reprojection fails
              weight *= underwater_params.w*(water_level-realWorldPos.y) < 0 ? saturate(1 - move * moveScale) : 1;
            #else
              weight *= saturate(1 - move * moveScale);
            #endif
          ##endif
        }

      half4 newTarget = 0;
      #if SSR_TRACEWATER == 1
        if (underwater_params.w * (realWorldPos.y - water_level) < 0)
          return 0;
        newTarget = performSSR(pixelPos, input.texcoord, linear_roughness, normal,
                               rawDepth, w, cameraToPoint, globtm_no_ofs_psf, water_level-realWorldPos.y+worldPos.y, worldPos);
      #else
        newTarget = performSSR(pixelPos, input.texcoord, linear_roughness, normal,
                               rawDepth, w, cameraToPoint, globtm_no_ofs_psf, 0, worldPos);
      #endif

      float4 prevTarget = tex2Dlod(ssr_prev_target, float4(oldUv, 0, 0));

      half4 result = force_ignore_history == 0 ? lerp(newTarget, prevTarget, weight) : newTarget;
      return result;
    }
  }
  compile("target_ps", "ssr_ps");
}

shader ssr_mipchain
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  USE_POSTFX_VERTEX_POSITIONS()

  (vs) { ssr_target_size@f4 = ssr_target_size; }
  (ps) { ssr_target@smp2d = ssr_target; }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 texcoord01     : TEXCOORD0;
      float4 texcoord23     : TEXCOORD1;
    };
  }

  hlsl(vs) {
    VsOutput ssr_mip_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos, 0, 1);
      float2 tc = screen_to_texcoords(pos);
      output.texcoord01 = tc.xyxy + 0.25*float4(-ssr_target_size.z,-ssr_target_size.w, ssr_target_size.z,-ssr_target_size.w);
      output.texcoord23 = tc.xyxy + 0.25*float4(-ssr_target_size.z, ssr_target_size.w, ssr_target_size.z, ssr_target_size.w);

      return output;
    }
  }

  hlsl(ps) {
    half4 ssr_mip_ps(VsOutput input) : SV_Target
    {
      half4 lt = tex2Dlod(ssr_target, float4(input.texcoord01.xy, 0,0));
      half4 rt = tex2Dlod(ssr_target, float4(input.texcoord01.zw, 0,0));
      half4 lb = tex2Dlod(ssr_target, float4(input.texcoord23.xy, 0,0));
      half4 rb = tex2Dlod(ssr_target, float4(input.texcoord23.zw, 0,0));

      return (lt+rt+lb+rb)*0.25;
    }
  }
  compile("target_vs", "ssr_mip_vs");
  compile("target_ps", "ssr_mip_ps");
}

shader ssr_only
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  hlsl(ps) {
    #define ENCODE_HIT_SKY
  }

  SSR_COMMON(ps)
  INIT_ZNZFAR()
  SSR_CALCULATE(ps)
  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  hlsl(ps) {
    half4 ssr_only_ps(VsOutput input) : SV_Target
    {
      half3 normal;
      half linear_roughness;
      float smoothness;
      unpack_material(input.texcoord, normal, linear_roughness, smoothness);

      // Early out
      float roughnessFade = saturate(linear_roughness* SSRParams.y + 2);
      BRANCH
      if (roughnessFade == 0)
        return 0;

      float rawDepth = tex2Dlod(depth_tex, float4(input.texcoord, 0, 0)).x;
      float w = linearize_z(rawDepth, zn_zfar.zw);

      BRANCH
      if (rawDepth <= 0)
        return 0;

      float3 cameraToPoint = input.viewVect.xyz * w;
      float3 worldPos = world_view_pos.xyz + cameraToPoint.xyz;
      float3 realWorldPos = ssr_world_view_pos.xyz + cameraToPoint.xyz;
      uint2 pixelPos = uint2(input.texcoord.xy * ssr_target_size.xy);
      #if SSR_TRACEWATER == 1
        if (underwater_params.w * (realWorldPos.y - water_level) < 0)
          return 0;
        half4 result = performSSR(pixelPos, input.texcoord, linear_roughness, normal,
                               rawDepth, w, cameraToPoint, globtm_no_ofs_psf, water_level - realWorldPos.y + worldPos.y, worldPos);
      #else
        half4 result = performSSR(pixelPos, input.texcoord, linear_roughness, normal,
                               rawDepth, w, cameraToPoint, globtm_no_ofs_psf, 0, worldPos);
      #endif

      return result;
    }
  }
  compile("target_ps", "ssr_only_ps");
}

shader ssr_resolve
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  SSR_COMMON(ps)
  INIT_ZNZFAR()
  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  (ps) {
    ssr_new_target@smp2d = ssr_target;
    prev_half_res_depth_tex@smp2d = prev_downsampled_far_depth_tex;

    ssr_filter0@f4 = ssr_filter0;
    ssr_filter1@f4 = ssr_filter1;
    ssr_filter2@f1 = (ssr_filter2);

    ssr_neighborhood_params@f4 = (ssr_clamping_gamma, ssr_neighborhood_bias_tight, ssr_neighborhood_bias_wide, ssr_neighborhood_velocity_difference_scale);
    ssr_inverse_screen_size@f2 = (ssr_target_size.z, ssr_target_size.w, 0, 0);
  }

  hlsl(ps) {
    #include <pixelPacking/yCoCgSpace.hlsl>

    #define ssr_clamping_gamma ssr_neighborhood_params.x
    #define ssr_neighborhood_bias_tight ssr_neighborhood_params.y
    #define ssr_neighborhood_bias_wide ssr_neighborhood_params.z
    #define ssr_neighborhood_velocity_difference_scale ssr_neighborhood_params.w

    // from temporalAA.hlsl
    void GatherCurrent(float gamma, float2 uv, float2 screenSizeInverse, out half4 current_color, out half4 color_min, out half4 color_max)
    {
      const int TAP_COUNT_NEIGHBORHOOD = 9;
      const float2 uvOffsets[TAP_COUNT_NEIGHBORHOOD] =
      {
        uv,
        uv + float2(-screenSizeInverse.x, 0.0),
        uv + float2( 0.0, -screenSizeInverse.y),
        uv + float2( screenSizeInverse.x, 0.0),
        uv + float2( 0.0, screenSizeInverse.y),
        uv + float2(-screenSizeInverse.x, -screenSizeInverse.y),
        uv + float2( screenSizeInverse.x, -screenSizeInverse.y),
        uv + float2(-screenSizeInverse.x, screenSizeInverse.y),
        uv + float2( screenSizeInverse.x, screenSizeInverse.y)
      };

      float4 sampleColors[TAP_COUNT_NEIGHBORHOOD];

      const float filterWeights[TAP_COUNT_NEIGHBORHOOD] =
      {
        ssr_filter0.x,
        ssr_filter0.y,
        ssr_filter0.z,
        ssr_filter0.w,
        ssr_filter1.x,
        ssr_filter1.y,
        ssr_filter1.z,
        ssr_filter1.w,
        ssr_filter2
      };

      const float INFINITY = 65535.0;
      color_min =  INFINITY;
      color_max = -INFINITY;
      half4 colorMoment1 = 0.0;
      half4 colorMoment2 = 0.0;
      half4 colorFiltered = 0.0;
      half  colorWeightTotal = 0.0;

      UNROLL
      for (int i = 0; i < TAP_COUNT_NEIGHBORHOOD; ++i)
      {
        half4 color = PackToYCoCgAlpha(tex2Dlod(ssr_new_target, float4(uvOffsets[i], 0, 0)));

        if (i == 4)
         current_color = color;

        colorFiltered += color * filterWeights[i];
        colorWeightTotal += filterWeights[i];

        colorMoment1 += color;
        colorMoment2 += color * color;
        color_min = min(color_min, color);
        color_max = max(color_max, color);
      }
      colorMoment1 /= (float)TAP_COUNT_NEIGHBORHOOD; // current average, mean
      colorMoment2 /= (float)TAP_COUNT_NEIGHBORHOOD; // mu

      half4 colorVariance = colorMoment2 - colorMoment1 * colorMoment1;
      half4 colorSigma = sqrt(max(0, colorVariance)) * gamma; // stddev * gamma
      half4 colorMin2 = colorMoment1 - colorSigma;
      half4 colorMax2 = colorMoment1 + colorSigma;

      colorMin2 = clamp(colorMin2, color_min, color_max);
      colorMax2 = clamp(colorMax2, color_min, color_max);

      color_min = colorMin2;
      color_max = colorMax2;

      half lumaContrast = color_max.x - color_min.x;
      colorFiltered /= colorWeightTotal;
      current_color = lerp(current_color, colorFiltered, saturate(lumaContrast));
    }

    // from temporalAA.hlsl
    half4 ClipHistory(half4 history, half4 current, half4 colorMin, half4 colorMax)
    {
      // Clip color difference against neighborhood min/max AABB
      float3 boxCenter = (colorMax.rgb + colorMin.rgb) * 0.5;
      float3 boxExtents = colorMax.rgb - boxCenter;

      float3 rayDir = current.rgb - history.rgb;
      float3 rayOrg = history.rgb - boxCenter;

      float clipLength = 1.0;

      if (length(rayDir) > 1e-6)
      {
        // Intersection using slabs
        float3 rcpDir = rcp(rayDir);
        float3 tNeg = ( boxExtents - rayOrg) * rcpDir;
        float3 tPos = (-boxExtents - rayOrg) * rcpDir;
        clipLength = saturate(max(max(min(tNeg.x, tPos.x), min(tNeg.y, tPos.y)), min(tNeg.z, tPos.z)));
      }

      return half4(lerp(history.rgb, current.rgb, clipLength), clamp(history.a, colorMin.a, colorMax.a));
    }

#ifdef PREV_MOTION_VECTORS_TEXTURE
    float get_velocity_weight(float2 current_velocity, float2 history_velocity)
    {
      float2 diff = current_velocity - history_velocity;
      return saturate((abs(diff.x) + abs(diff.y)) * ssr_neighborhood_velocity_difference_scale);
    }

    float get_neighborhood_bias_from_velocity_weight(float2 motion, float2 historyUv)
    {
      float2 prevRawMotion = tex2Dlod(PREV_MOTION_VECTORS_TEXTURE, float4(historyUv, 0, 0)).rg;
      float2 prevMotion = decode_motion_vector(prevRawMotion);
      float velocityWeight = get_velocity_weight(motion, prevMotion);
      return lerp(ssr_neighborhood_bias_wide, ssr_neighborhood_bias_tight, velocityWeight);
    }
#endif

    float get_disocclusion(float current_linear_depth, float prev_linear_depth)
    {
      const float disocclusionDepthScale = 1.0f;
      return saturate(disocclusionDepthScale * (current_linear_depth - prev_linear_depth));
    }

    float get_neighborhood_bias_from_depth_disocclusion(float depth, float2 historyUv)
    {
      float prevDepth = linearize_z(tex2Dlod(prev_half_res_depth_tex, float4(historyUv, 0, 0)).r, zn_zfar.zw);
      float disocclusion = get_disocclusion(depth, prevDepth);
      return lerp(ssr_neighborhood_bias_wide, ssr_neighborhood_bias_tight, disocclusion);
    }

    half4 ssr_resolve_ps(VsOutput input) : SV_Target
    {
      float rawDepth = tex2Dlod(depth_tex, float4(input.texcoord, 0, 0)).x;
      float depth = linearize_z(rawDepth, zn_zfar.zw);

      // get history uv and neighborhood bias from either motion vectors or reprojection
      float2 historyUv;
      float neighborhoodBias;
      #if SSR_MOTIONREPROJ == 1
        float2 rawMotion = tex2Dlod(MOTION_VECTORS_TEXTURE, float4(input.texcoord, 0, 0)).rg;
        BRANCH if (CHECK_VALID_MOTION_VECTOR(rawMotion))
        {
          float2 motion = decode_motion_vector(rawMotion);
          historyUv = input.texcoord + motion;
          #ifdef PREV_MOTION_VECTORS_TEXTURE
            neighborhoodBias = get_neighborhood_bias_from_velocity_weight(motion, historyUv);
          #else
            neighborhoodBias = get_neighborhood_bias_from_depth_disocclusion(depth, historyUv);
          #endif
        }
        else
      #endif
        {
          float3 cameraToPoint = input.viewVect * depth;
          #if SSR_MOTIONREPROJ != 1
            apply_hero_matrix(cameraToPoint);
          #endif
          historyUv = get_reprojected_history_uv(cameraToPoint, prev_globtm_no_ofs_psf);
          neighborhoodBias = get_neighborhood_bias_from_depth_disocclusion(depth, historyUv);
        }

      // current with neighborhood
      half4 currentColor, colorMin, colorMax;
      GatherCurrent(ssr_clamping_gamma, input.texcoord.xy, ssr_inverse_screen_size, currentColor, colorMin, colorMax);
      colorMin -= neighborhoodBias;
      colorMax += neighborhoodBias;

      // history
      float4 previousColor = tex2Dlod(ssr_prev_target, float4(historyUv, 0, 0));

      // clip history with neighborhood
      previousColor = PackToYCoCgAlpha(previousColor);
      half4 clippedHistory = ClipHistory(previousColor, currentColor, colorMin, colorMax);

      float4 result = lerp(currentColor, clippedHistory, 0.9);
      result.rgb = UnpackFromYCoCg(result.rgb);
      return result;
    }
  }
  compile("target_ps", "ssr_resolve_ps");
}

shader ssr_vtr
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  INIT_ZNZFAR()
  GET_ALTERNATE_REFLECTIONS(ps)
  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)
  INIT_TEXTURES(ps)

  (ps) {
    ssr_strength@smp2d = ssr_strength;
    vtr_target_size@f2 = vtr_target_size;
    SSRParams@f4 = (SSRParams.x, SSRParams.y, ssr_frameNo.z, ssr_frameNo.y);
    world_view_pos@f4 = world_view_pos;
  }

  hlsl(ps) {
    half4 ssr_vtr_ps(VsOutput input) : SV_Target
    {
      float rawDepth = tex2Dlod(depth_tex, float4(input.texcoord, 0, 0)).x;
      BRANCH if (rawDepth <= 0)
        return 0;

      // Since VTR is rendered half the resolution as SSR, taking one bilinear sample of SSR here will blend 4 SSR
      // texels with uniform weight. This works for us just fine because any texel in the 2x2 grid having < 1 alpha will
      // drag down alpha value causing us to render VTR on this pixel.
      float4 ssrValue = tex2Dlod(ssr_strength, float4(input.texcoord, 0, 0));
      BRANCH if (ssrValue.a >= 1.0)
        return 0;

      // if SSR hits the sky and is not vignetted, no need to render VTR.
      BRANCH if (ssrValue.a == 0.0 && ssrValue.b == 0.0)
        return 0;

      half4 result = 0;
      uint2 pixelPos = uint2(input.texcoord * vtr_target_size);
      float w = linearize_z(rawDepth, zn_zfar.zw);
      float3 cameraToPoint = input.viewVect.xyz * w;
      half3 normal = tex2Dlod(normals_tex, float4(input.texcoord, 0, 0)).xyz * 2 - 1;
      get_alternate_reflections(result, pixelPos, cameraToPoint, normal, false, false);
      // if SSR hit the sky, fadeout value from b component is used (see end of ssr_common.hlsl)
      BRANCH if (ssrValue.a == 0)
        result.rgba *= ssrValue.b;
      return result;
    }
  }
  compile("target_ps", "ssr_vtr_ps");
}

shader ssr_composite
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  POSTFX_VS_TEXCOORD(0, texcoord)

  (ps) {
    ssr_target@smp2d = ssr_target;
    vtr_target@smp2d = vtr_target;
  }

  hlsl(ps) {
    half4 ssr_composite_ps(VsOutput input) : SV_Target
    {
      float4 ssr = tex2Dlod(ssr_target, float4(input.texcoord, 0, 0));
      float4 vtr = tex2Dlod(vtr_target, float4(input.texcoord, 0, 0));

      // 0 is hit sky, see the end of ssr_common.hlsl
      ssr.a = saturate((ssr.a - 1.0 / 255.0) * 255.0 / 254.0);
      // Choosing maximum alpha prevents a seam of <1 alpha values to form where VTR and SSR is blended
      return float4(lerp(vtr.rgb, ssr.rgb, ssr.a), max(ssr.a, vtr.a));
    }
  }
  compile("target_ps", "ssr_composite_ps");
}