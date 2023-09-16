include "cloudsLighting.sh"
include "cloudsShadowVolume.sh"
include "skyLight.sh"
//include "sky_shader_global.sh"
//same as:
include "hardware_defines.sh"
include "writeToTex.sh"

hlsl {
  float clampedPow(float X,float Y) { return pow(max(abs(X),0.000001f),Y); }
  half luminance(half3 col)  { return dot(col, half3(0.299, 0.587, 0.114)); }
}
float4 skies_primary_sun_light_dir = (0,0.4,0.6,0);
float skies_planet_radius = 6360;//replace with planet_radius!
//--

int cloud_shadow_samples_cnt = 40;//

float4 clouds_start_compute_offset;
float4 clouds_compute_width;
float4 clouds_new_shadow_ambient_weight;
texture cloud_shadows_old_values;

macro INIT_COPY_CLOUD_SHADOWS_VOLUME(stage)
  (stage) {
    clouds_new_shadow_ambient_weight@f2 = clouds_new_shadow_ambient_weight;
    start_compute_offset@f3 = clouds_start_compute_offset;
    clouds_compute_width@f2 = clouds_compute_width;
    clouds_shadows_volume@smp3d = clouds_shadows_volume;
  }
  hlsl(stage) {
    #include <cloud_settings.hlsli>
    uint encode(float v, uint shift) {return uint(v*255+0.5)<<shift;}
  }
endmacro

shader copy_cloud_shadows_volume
{
  INIT_COPY_CLOUD_SHADOWS_VOLUME(cs)
  hlsl(cs) {
    RWTexture3D<uint> output : register(u0);

    [numthreads(CLOUD_SHADOWS_WARP_SIZE_XZ/2, CLOUD_SHADOWS_WARP_SIZE_XZ, CLOUD_SHADOWS_WARP_SIZE_Y)]
    void cs_main(uint3 dtid : SV_DispatchThreadID) {
      uint3 tid = uint3(dtid.x*2, dtid.yz) + uint3(start_compute_offset.xyz);//todo: use some jitter instead (z-curve)
      float4 currentValues;
      currentValues.xy = clouds_shadows_volume[tid].xy;
      currentValues.zw = clouds_shadows_volume[tid+uint3(1,0,0)].xy;
      output[dtid] = encode(currentValues.x,0) | encode(currentValues.y,8) | encode(currentValues.z,16) | encode(currentValues.w,24);
    }
  }
  compile("cs_5_0", "cs_main")
}

shader copy_cloud_shadows_volume_ps
{
  if (hardware.metal)
  {
    dont_render;
  }
  INIT_COPY_CLOUD_SHADOWS_VOLUME(ps)
  WRITE_TO_VOLTEX_TC()
  hlsl(ps) {
    uint ps_main(VsOutput input HW_USE_SCREEN_POS): SV_Target0
    {
      uint3 dtid = dispatchThreadID(input);
      uint3 tid = uint3(dtid.x*2, dtid.yz) + uint3(start_compute_offset.xyz);//todo: use some jitter instead (z-curve)
      float4 currentValues;
      currentValues.xy = clouds_shadows_volume[tid].xy;
      currentValues.zw = clouds_shadows_volume[tid+uint3(1,0,0)].xy;
      return encode(currentValues.x,0) | encode(currentValues.y,8) | encode(currentValues.z,16) | encode(currentValues.w,24);
    }
  }
  compile("target_ps", "ps_main")
}

macro USE_GEN_CLOUD_SHADOWS_VOLUME(stage)
  hlsl {
    #define SIMPLIFIED_ALT_FRACTION 1
  }
  //RAY_INTERSECT_TWO()
  local float4 offseted_view_pos  = (0,0,0,0);
  SAMPLE_CLOUDS_DENSITY_TEXTURE(stage, offseted_view_pos)
  local float low_sun_mul = (1-max(skies_primary_sun_light_dir.z, -skies_primary_sun_light_dir.z));
  (stage) {
    skies_primary_sun_light_dir@f3 = skies_primary_sun_light_dir;
    shadow_step@f3 = (128 + (512-128)*low_sun_mul, 0.0078125 + 0.0078125*8*low_sun_mul*low_sun_mul, 64, 0);
  }
  INIT_SKY_DIFFUSE_BASE(stage)
  USE_SKY_DIFFUSE_BASE(stage)

  CLOUDS_LIGHTING_COMMON(stage)
  local float cloud_start_radius = (skies_planet_radius+clouds_start_altitude2);
  local float cloud_end_radius = (cloud_start_radius+clouds_thickness2);
  (stage) {
    clouds_radiuses@f4 = (cloud_start_radius*cloud_start_radius, cloud_end_radius*cloud_end_radius,-skies_planet_radius, cloud_end_radius);
    clouds_altitudes@f2 = (clouds_start_altitude2, clouds_start_altitude2+clouds_thickness2, 0,0);
    clouds_alt_to_world@f2 = (clouds_thickness2*1000, clouds_start_altitude2*1000,0,0);
    skies_planet_radius@f1 = (skies_planet_radius);
    cloud_shadow_samples_cnt@f2 = (cloud_shadow_samples_cnt, 8,0,0);
    start_compute_offset@f3 = clouds_start_compute_offset;
  }
  if (shader == gen_cloud_shadows_volume_partial_cs || shader == gen_cloud_shadows_volume_partial_ps)
  {
    (stage) {
      clouds_new_shadow_ambient_weight@f2 = clouds_new_shadow_ambient_weight;
      clouds_compute_width@f2 = clouds_compute_width;
      oldValues@smp = cloud_shadows_old_values hlsl {Texture3D<uint> oldValues@smp;}
    }
    hlsl(stage) {
      #define TEMPORAL_UPDATE_SHADOW 1
      float2 get_old_values(uint3 dtid)
      {
        uint a = oldValues[uint3(dtid.x / 2, dtid.yz)];
        if (dtid.x&1)
          a = a>>16;
        return float2((a&0xFF)/255.0, ((a>>8)&0xFF)/255.0);
      }
    }
  }
endmacro

macro GEN_CLOUD_SHADOWS_VOLUME_MAIN(stage)
  hlsl(stage)
  {
    #include <monteCarlo.hlsl>
    #include <fibonacci_sphere.hlsl>
    uint3 gen_cloud_shadows_volume_main(in uint3 dtid, in uint3 gid, out float2 result)
    {
      //todo: toroidal update for additional around
      uint shadowSamples = cloud_shadow_samples_cnt.x;
      float shadowStep = shadow_step.x;
      float expStep = shadow_step.y;

      uint firstSampleCount = cloud_shadow_samples_cnt.y;
      float firstShadowStep = shadow_step.z;
      //shadowSamples += shadowSamples;
      //shadowStep*=0.5;
      //expStep*=0.75;
      #if TEMPORAL_UPDATE_SHADOW
        //partial update implementation (temporal)
        uint3 tid = dtid + uint3(start_compute_offset.xyz);//todo: use some jitter instead (z-curve)
        if (gid.z == 0 && start_compute_offset.z == 0)
        {
          shadowSamples += shadowSamples;
          shadowStep*=0.75;
        }
      #else
        //support offset for splitting workload on mobile GPUs
        uint3 tid = dtid + uint3(start_compute_offset.xyz);
        if (gid.z == 0)
        {
          shadowSamples += shadowSamples;
          shadowStep*=0.75;//expStep*=0.75;
        }
      #endif
      float3 worldPos;
      #if SHADOW_VOLUME_AROUND
      worldPos.xz = (tid.xy+0.5-CLOUD_SHADOWS_VOLUME_RES_XZ/2)*CLOUDS_SHADOW_VOL_TEXEL + floor(skies_world_view_pos.xz/CLOUDS_SHADOW_VOL_TEXEL+0.5)*CLOUDS_SHADOW_VOL_TEXEL;
      #else
      worldPos.xz = ((tid.xy+0.5f)/float(CLOUD_SHADOWS_VOLUME_RES_XZ)-0.5)*WEATHER_SIZE;
      #endif
      float heightFraction = (tid.z+0.5)/CLOUD_SHADOWS_VOLUME_RES_Y;
      worldPos.y = heightFraction*clouds_alt_to_world.x + clouds_alt_to_world.y;

      //todo: add planet shadow!
      float light_sample_extinctionLast = 1.0;
      //first few steps are independent on light direction, with a fixed step size.
      //that is to keep lighting within the cloud 'rich'
      baseLightExtinction(light_sample_extinctionLast, worldPos, heightFraction, firstShadowStep,
         0,//mip
         0,//add_mip
         0.25,//erosion level
         0.01,//threshold
         skies_primary_sun_light_dir.xzy,
         firstSampleCount,
         0
       );

      baseLightExtinction(light_sample_extinctionLast, worldPos + skies_primary_sun_light_dir.xzy*firstSampleCount*firstShadowStep, heightFraction, shadowStep,
         0,//mip
         1.5/shadowSamples,//add_mip
         0.25,//erosion level
         0.01,//threshold
         skies_primary_sun_light_dir.xzy,
         shadowSamples,
         expStep
       );



      float ambient_extinction = 0.0;
      #if TEMPORAL_UPDATE_SHADOW
      BRANCH
      if (clouds_new_shadow_ambient_weight.y > 0)
      #endif
      {
        float ambientStepSize = 96;
        float baseMip = 0, mipAdd = 0.25;
        #if MOBILE_DEVICE
          //~5 times faster on nswitch (1,2s -> 0,25s)
          int NUM_AMBIENT_SAMPLES = 8;
        #else
          int NUM_AMBIENT_SAMPLES = 16;
        #endif
        half totalWeight = 0;
        //very expensive ambient computations

        for (int smpl = 0; smpl < NUM_AMBIENT_SAMPLES; ++smpl)
        {
          float3 cDir = uniform_sample_sphere(fibonacci_sphere(smpl, NUM_AMBIENT_SAMPLES)).xyz;//todo: can be just constants
          float ambientDirExtinction = 1.0;
          baseLightExtinction(ambientDirExtinction, worldPos, heightFraction, ambientStepSize, baseMip, mipAdd, 0.5, 0.05, cDir, 6, 0.25);
          float amb = dot(getMSExtinction(ambientDirExtinction), clouds_ms_contribution4 . octaves_attributes);
          float groundScale = cDir.y>0 ? 1 : 0.333;//ground is at least 3 times darker then sky

          float horizonWeight = 1+0.25*saturate(1-abs(cDir.y));//horizon is usually brighter, due to mie
          horizonWeight *= horizonWeight;
          totalWeight += horizonWeight;
          ambient_extinction += amb*groundScale*horizonWeight;
        }

        ambient_extinction *= 4.0/totalWeight;//1/4 is phase function, we need to remove it
      }
      result = float2(light_sample_extinctionLast, ambient_extinction);

      #if TEMPORAL_UPDATE_SHADOW
        float2 currentValues = get_old_values(dtid);
        result.x = lerp(currentValues.x, result.x, clouds_new_shadow_ambient_weight.x);
        result.y = lerp(currentValues.y, result.y, clouds_new_shadow_ambient_weight.y);
      #endif
      return tid;
    }
  }
endmacro

shader gen_cloud_shadows_volume_cs, gen_cloud_shadows_volume_partial_cs
{
  USE_GEN_CLOUD_SHADOWS_VOLUME(cs)
  hlsl(cs) {
    #define D3D_SHADER_REQUIRES_TYPED_UAV_LOAD_ADDITIONAL_FORMATS 1
    RWTexture3D<float2> output : register(u0);
  }
  GEN_CLOUD_SHADOWS_VOLUME_MAIN(cs)
  hlsl(cs) {
    [numthreads(CLOUD_SHADOWS_WARP_SIZE_XZ, CLOUD_SHADOWS_WARP_SIZE_XZ, CLOUD_SHADOWS_WARP_SIZE_Y)]
    void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gid:SV_GroupID) {
      float2 result;
      uint3 tid = gen_cloud_shadows_volume_main(dtid, gid, result);
      output[tid] = result;
    }
  }
  compile("cs_5_0", "cs_main")
}

shader gen_cloud_shadows_volume_ps, gen_cloud_shadows_volume_partial_ps
{
  if (hardware.metal)
  {
    dont_render;
  }
  WRITE_TO_VOLTEX_TC()
  USE_GEN_CLOUD_SHADOWS_VOLUME(ps)
  GEN_CLOUD_SHADOWS_VOLUME_MAIN(ps)
  hlsl(ps) {
    float2 ps_main(VsOutput input HW_USE_SCREEN_POS): SV_Target0
    {
      uint3 dtid = dispatchThreadID(input);
      #if TEMPORAL_UPDATE_SHADOW
        dtid.z -= uint(start_compute_offset.z);
      #endif
      uint3 gid = dtid / uint3(CLOUD_SHADOWS_WARP_SIZE_XZ, CLOUD_SHADOWS_WARP_SIZE_XZ, CLOUD_SHADOWS_WARP_SIZE_Y);
      float2 result;
      uint3 tid = gen_cloud_shadows_volume_main(dtid, gid, result);
      return result;
    }
  }
  compile("target_ps", "ps_main")
}
