texture depth_around;
texture depth_around_transparent;
texture blurred_depth;
texture blurred_depth_transparent;

float4 world_to_depth_ao;
float4 depth_ao_heights;
float depth_ao_height_scale = 1;

float depth_ao_texture_size;
float depth_ao_texture_rcp_size;

macro INIT_BLURRED_DEPTH_ABOVE_CONSTS(code)
  (code) {
    world_to_depth_ao@f4 = world_to_depth_ao;
    depth_ao_heights@f4 = depth_ao_heights;
    depth_ao_texture_sizes@f2 = (depth_ao_texture_size, depth_ao_texture_rcp_size, 0, 0);
  }
endmacro

macro INIT_DEPTH_ABOVE(code, tex)
  (code) {tex@smp2d = tex;}
  INIT_BLURRED_DEPTH_ABOVE_CONSTS(code)
endmacro

macro INIT_BLURRED_DEPTH_ABOVE(code)
  INIT_DEPTH_ABOVE(code, blurred_depth)
endmacro

macro INIT_DEPTH_ABOVE_WITHOUT_SAMPLER(code, tex_name, tex_samplerstate, samplerstate)
  (code) { tex_name@tex = tex_name hlsl { Texture2D tex_name@tex; } }
  hlsl(code) {
    #define tex_samplerstate samplerstate
  }
  INIT_BLURRED_DEPTH_ABOVE_CONSTS(code)
endmacro

macro INIT_BLURRED_DEPTH_ABOVE_WITHOUT_SAMPLER(code, samplerstate)
  INIT_DEPTH_ABOVE_WITHOUT_SAMPLER(code, blurred_depth, blurred_depth_samplerstate, samplerstate)
endmacro

macro USE_DEPTH_ABOVE_TC(code)
  hlsl(code){

    float2 getWorldBlurredDepthTC(float3 worldPos, out float vignette_effect)
    {
      float2 tc = saturate(world_to_depth_ao.xy*worldPos.xz + world_to_depth_ao.zw);
      float2 vignette = saturate( abs(tc*2-1) * 10 - 9 );
      vignette_effect = saturate(dot( vignette, vignette ));
      return tc - depth_ao_heights.zw;
    }
    float decode_depth_above(float depthHt)
    {
      return depthHt*depth_ao_heights.x+depth_ao_heights.y;
    }
    float4 decode_depth_above4(float4 depthHt)
    {
      return depthHt*depth_ao_heights.x+depth_ao_heights.y;
    }
  }
endmacro

macro USE_DEPTH_ABOVE(code, tex)
  USE_DEPTH_ABOVE_TC(code)
  hlsl(code){
    float getWorldBlurredDepth(float3 worldPos, out float vignette_effect)
    {
      float2 tc = getWorldBlurredDepthTC(worldPos, vignette_effect);
      return decode_depth_above(tex2Dlod(tex, float4(tc,0,0) ).x);
    }
    float4 gatherWorldBlurredDepth(float3 worldPos, out float vignette_effect, out float2 lerp_factor)
    {
      float2 tc = getWorldBlurredDepthTC(worldPos, vignette_effect);

      tc = frac(tc); //to emulate wrap addressing
      float2 coordF = tc * depth_ao_texture_sizes.xx - 0.5;
      lerp_factor  = frac(coordF);

      float2 centerTc = (floor(coordF) + 1.0) * depth_ao_texture_sizes.yy;
      return decode_depth_above4(textureGather(tex, centerTc));
    }
    float lerpGatheredBlurredDepth(float4 depth_samples, float2 lerp_factor)
    {
      float ctop = lerp(depth_samples.w, depth_samples.z, lerp_factor.x);
      float cbot = lerp(depth_samples.x, depth_samples.y, lerp_factor.x);

      return lerp(ctop, cbot, lerp_factor.y);
    }
  }
endmacro

macro USE_BLURRED_DEPTH_ABOVE(shader_code)
  USE_DEPTH_ABOVE(shader_code, blurred_depth)
endmacro

macro INIT_DEPTH_AO_STAGE(code)
  INIT_WORLD_HEIGHTMAP_BASE(code)
  INIT_BLURRED_DEPTH_ABOVE(code)
endmacro

macro INIT_DEPTH_AO()
  INIT_DEPTH_AO_STAGE(ps)
endmacro

macro USE_DEPTH_AO_STAGE(code)
  USE_BLURRED_DEPTH_ABOVE(code)
  USE_HEIGHTMAP_COMMON_BASE(code)
  (code) {
        depth_ao_height_scale@f1 = (depth_ao_height_scale);
  }
  hlsl(code) {
    float getWorldBlurredAO(float3 worldPos)
    {
      float vignetteEffect;
      float depthHt = getWorldBlurredDepth(worldPos, vignetteEffect);
      const float height_bias = 0.05;
##if tex_hmap_low != NULL
      const float height_scale = 1.0f;
##else
      const float height_scale = 0.5f;
##endif
      float occlusion = rcp((max(0.01, (depthHt - height_bias - worldPos.y)*(height_scale * depth_ao_height_scale)+1)));
      float ao = saturate(occlusion)*0.9 + 0.1;
##if tex_hmap_low != NULL
      float worldD = getWorldHeight(worldPos.xz) + 0.01;
      const float max_height_above_heightmap = 10.f;
      ao = lerp(1, ao, pow2(1 - saturate((1. / max_height_above_heightmap)*(worldPos.y - worldD))));
##endif
      return lerp(ao, 1, vignetteEffect);
    }
  }
endmacro

macro USE_DEPTH_AO()
  USE_DEPTH_AO_STAGE(ps)
endmacro
