include "shader_global.sh"
include "viewVecVS.sh"
include "gbuffer.sh"
include "contact_shadows.sh"
include "ssao_use.sh"
include "get_additional_shadows.sh"
include "get_additional_ao.sh"
include "ssao_reprojection.sh"

float gtao_radius = 2.8;
float gtao_temporal_directions = 0;
float gtao_temporal_offset = 0;
float4 hero_cockpit_vec = (0, 0, 0, 0);
texture raw_gtao_tex;
texture gtao_tex;
texture gtao_prev_tex;
texture prev_downsampled_far_depth_tex;
texture downsampled_checkerboard_depth_tex;
texture downsampled_close_depth_tex;
texture downsampled_normals;
float4 globtm_no_ofs_psf_0;
float4 globtm_no_ofs_psf_1;
float4 globtm_no_ofs_psf_2;
float4 globtm_no_ofs_psf_3;

macro INIT_HALF_RES_DEPTH(stage)
  if (downsampled_close_depth_tex != NULL)
  {
    // This is not correct, as half-res SSAO will be upsampled with the assumption that it was rendered with
    // checkerboard_depth, but using close_depth helps in reducing bright halo around edges of objects in front of
    // occluded areas. In the long run we need to figure out how to prevent this with checkerboard_depth, or use proper
    // uplampling based on close_depth for SSAO
    (stage) { half_res_depth_tex@smp2d = downsampled_close_depth_tex; }
  }
  else if (downsampled_checkerboard_depth_tex != NULL)
  {
    (stage) { half_res_depth_tex@smp2d = downsampled_checkerboard_depth_tex; }
  }
  else
  {
    (stage) { half_res_depth_tex@smp2d = downsampled_far_depth_tex; }
  }
endmacro

define_macro_if_not_defined USE_READ_GBUFFER_MATERIAL_DYNAMIC_BASE(stage)
endmacro

define_macro_if_not_defined INIT_READ_GBUFFER_MATERIAL_DYNAMIC_BASE(stage)
endmacro

hlsl{
  float decode_depth(float depth, float2 decode_depth)
  {
    return linearize_z(depth,decode_depth);
  }
}

int gtao_tex_size;
interval gtao_tex_size: low<300, mid<400, high;

macro GTAO_MAIN_CORE(code)
  supports none;
  supports global_const_block;

  local float4 halfResTexSize = get_dimensions(downsampled_far_depth_tex, 0);
  (code) {
    world_view_pos@f3 = world_view_pos;
    gtao_radius@f1 = (gtao_radius);
    gtao_texel_size@f4 = (1.0 / halfResTexSize.xy, halfResTexSize.xy);
    gtao_temporal_directions@f1 = (gtao_temporal_directions);
    gtao_temporal_offset@f1 = (gtao_temporal_offset);
    gtao_height_width_rel@f1 = (halfResTexSize.y / halfResTexSize.x, 0, 0, 0);
    hero_cockpit_vec@f4 = hero_cockpit_vec;
    //todo: use just downsampled_close_depth_tex
    //however, it has to be changed in USE_SSAO simultaneously, and in spatial filter, and in temporal filter, including prev_depth
    // (and probably in SSR)
    //for now, just use both
    downsampled_far_depth_tex@smp2d = downsampled_far_depth_tex;
    downsampled_close_depth_tex@smp2d = downsampled_close_depth_tex;
    downsampled_normals@smp2d = downsampled_normals;

    local_view_x@f3 = local_view_x;
    local_view_y@f3 = local_view_y;
    local_view_z@f3 = local_view_z;

    shadow_params@f3 = (0, shadow_frame, contact_shadow_len, 0);

    viewProjectionMatrixNoOfs@f44 = { globtm_no_ofs_psf_0, globtm_no_ofs_psf_1, globtm_no_ofs_psf_2, globtm_no_ofs_psf_3 };

    encode_depth@f4 = (-zn_zfar.x / (zn_zfar.y - zn_zfar.x), zn_zfar.x * zn_zfar.y / (zn_zfar.y - zn_zfar.x),
      zn_zfar.x / (zn_zfar.x * zn_zfar.y), (zn_zfar.y - zn_zfar.x) / (zn_zfar.x * zn_zfar.y));
  }

  INIT_HALF_RES_DEPTH(code)

  INIT_ZNZFAR_STAGE(code)
  hlsl(code) {
    #define shadow_frame shadow_params.y
    #define contact_shadow_len shadow_params.z
    #define USE_LINEAR_THRESHOLD 1
  }

  CONTACT_SHADOWS_BASE(code)
  APPLY_ADDITIONAL_AO(code)
  APPLY_ADDITIONAL_SHADOWS(code)
  USE_PACK_SSAO_BASE(code)
  INIT_READ_GBUFFER_MATERIAL_DYNAMIC_BASE(code)
  USE_READ_GBUFFER_MATERIAL_DYNAMIC_BASE(code)
  ENABLE_ASSERT(code)

  hlsl(code) {

    half3 GetCameraVec(half2 uv)
    {
      return half3(uv.x * -2.0 + 1.0, uv.y * 2 * gtao_height_width_rel - gtao_height_width_rel, 1.0);
    }

    half3 GetPosition(half2 uv)
    {
      float sceneDepth = decode_depth(tex2Dlod(downsampled_close_depth_tex, float4(uv.xy, 0, 0)).x, zn_zfar.zw);
      return GetCameraVec(uv) * sceneDepth;
    }

    inline half GTAO_Offsets(half2 uv)
    {
      int2 position = (int2)(uv * gtao_texel_size.zw);
      return 0.25 * (half)((position.y - position.x) & 3);
    }

    inline half GTAO_Directions(half2 uv)
    {
      // directions use a 4x4 pattern that contains a full rotation in each row and is shifted left/right between rows
      int2 position = (int2)(uv * gtao_texel_size.zw);
      return (1.0 / 16.0) * ((((position.x + position.y) & 3) << 2) + (position.x & 3));
    }

    #define SSAO_LIMIT 200
    #define SSAO_SAMPLES 4
    #define SSAO_FALLOFF 2.25
    #define SSAO_THICKNESSMIX 0.2
    #ifdef READ_W_DEPTH //WT branch
      #define SSAO_MAX_STRIDE 4
    #else //Enlisted branch
      #define SSAO_MAX_STRIDE 16
    #endif
    #define PI_HALF 1.5707963267948966192313216916398

    void SliceSample(half2 tc_base, half2 aoDir, int i, float targetMip, half3 ray, half3 v, inout float closest)
    {
      half2 uv = clamp(tc_base + aoDir * i,float2(0.001,0.001),float2(0.999,0.999));
      float rawDepth = tex2Dlod(downsampled_close_depth_tex, float4(uv.xy, 0, targetMip)).x;

      ##if (use_screen_mask == yes)
        if (rawDepth >= 1)
          return;
      ##endif

      float depth = decode_depth(rawDepth, zn_zfar.zw);
      // Vector from current pixel to current slice sample
      half3 p = GetCameraVec(uv) * depth - ray;
      // Cosine of the horizon angle of the current sample
      float current = dot(v, normalize(p));
      // Linear falloff for samples that are too far away from current pixel
      float falloff = saturate((gtao_radius - length(p)) / SSAO_FALLOFF);
      FLATTEN
      if (current > closest)
        closest = lerp(closest, current, falloff);
      // Helps avoid overdarkening from thin objects
      closest = lerp(closest, current, SSAO_THICKNESSMIX * falloff);
    }

    float GTAOFastAcos(float x)
    {
      float res = -0.156583 * abs(x) + PI_HALF;
      res *= sqrt(1.0 - abs(x));
      return x >= 0 ? res : PI - res;
    }

    float IntegrateArc(float h1, float h2, float n)
    {
      float cosN = cos(n);
      float sinN = sin(n);
      return 0.25 * (-cos(2.0 * h1 - n) + cosN + 2.0 * h1 * sinN - cos(2.0 * h2 - n) + cosN + 2.0 * h2 * sinN);
    }

    half gtao(half2 tc_original, half3 worldNormal)
    {

      // Vector from camera to the current pixel's position
      half3 ray = GetPosition(tc_original);

      float3 normal = normalize(float3(
        -dot(local_view_x, worldNormal),
        -dot(local_view_y, worldNormal),
         dot(local_view_z, worldNormal)
      ));

      // Calculate the distance between samples (direction vector scale) so that the world space AO radius remains constant but also clamp to avoid cache trashing
      float stride = min((1.0 / length(ray)) * SSAO_LIMIT, SSAO_MAX_STRIDE);
      half2 dirMult = gtao_texel_size.xy * stride;
      // Get the view vector (normalized vector from pixel to camera)
      half3 v = normalize(-ray);

      // Calculate slice direction from pixel's position
      float noiseDirection = GTAO_Directions(tc_original);
      float angle = (noiseDirection + gtao_temporal_directions) * PI * 2;
      half2 aoDir = dirMult * half2(sin(angle), cos(angle));

      // Project world space normal to the slice plane
      half3 toDir = GetCameraVec(tc_original + aoDir);
      half3 planeNormal = normalize(cross(v, -toDir));
      half3 projectedNormal = normal - planeNormal * dot(normal, planeNormal);

      // Calculate angle n between view vector and projected normal vector
      half3 projectedDir = normalize(normalize(toDir) + v);
      float n = GTAOFastAcos(dot(-projectedDir, normalize(projectedNormal))) - PI_HALF;

      // Init variables
      float c1 = -1.0;
      float c2 = -1.0;

      half noiseOffset = GTAO_Offsets(tc_original) + gtao_temporal_offset;
      half2 tc_base = tc_original + aoDir * (0.125 * noiseOffset - 0.375);

      // Mipmapping is disabled as it is causing severe visual errors. See https://youtrack.gaijin.ru/issue/38-7328
      const float targetMip = 0.0;

      // Find horizons of the slice
      int i;
      for (i = -1; i >= -SSAO_SAMPLES; i--)
      {
        SliceSample(tc_base, aoDir, i, targetMip, ray, v, c1);
      }
      for (i = 1; i <= SSAO_SAMPLES; i++)
      {
        SliceSample(tc_base, aoDir, i, targetMip, ray, v, c2);
      }

      // Finalize
      float h1a = -GTAOFastAcos(c1);
      float h2a = GTAOFastAcos(c2);

      // Clamp horizons to the normal hemisphere
      float h1 = n + max(h1a - n, -PI_HALF);
      float h2 = n + min(h2a - n, PI_HALF);

      float visibility = IntegrateArc(h1, h2, n) * length(projectedNormal);

      return visibility;
    }

    SSAO_TYPE gtao_resolve(float2 screenpos, float2 texcoord, float3 viewVect)
    {
      SSAO_TYPE gtao_value;
      #if _HARDWARE_METAL && !SHADER_COMPILER_DXC
        #define GTAO_VALUE_X gtao_value
      #else
        #define GTAO_VALUE_X gtao_value.x
      #endif
      float rawDepth = tex2Dlod(half_res_depth_tex, float4(texcoord, 0, 0)).x;
      float depth1 = decode_depth(rawDepth, zn_zfar.zw);
      BRANCH
      if (rawDepth >= 1 || depth1 >= zn_zfar.y*0.99)
        return 1;

      half3 wsNormal = normalize(tex2Dlod(downsampled_normals, float4(texcoord, 0, 0)).xyz * 2 - 1);
      float3 cameraToPoint = depth1 * viewVect;
      float w = depth1;
      float3 pixelPos = cameraToPoint + world_view_pos;

      GTAO_VALUE_X = gtao(texcoord, wsNormal);
      GTAO_VALUE_X = getAdditionalAmbientOcclusion(float(GTAO_VALUE_X), pixelPos, wsNormal, texcoord);

      ##if maybe(ssao_contact_shadows)
      {
        ##if gtao_tex_size == low
          #define shadow_steps 8
        ##elif gtao_tex_size == mid
          #define shadow_steps 16
        ##else
          #define shadow_steps 24
        ##endif

        float dither = interleavedGradientNoiseFramed(screenpos, floor(shadow_frame));//if we have temporal aa
        dither = interleavedGradientNoise(screenpos);
        float2 hitUV;
        float offset = 0.0001;
        float sunNdotL = dot(wsNormal, from_sun_direction);
        float sunNoL = abs(sunNdotL);
        offset = lerp(0.05, 0.0001, sunNoL);
        float fovScale = 1.0;
        gtao_value.CONTACT_SHADOWS_ATTR = contactShadowRayCast(
          downsampled_far_depth_tex, downsampled_far_depth_tex_samplerstate,
          cameraToPoint - normalize(viewVect) * (w * offset + 0.005), -from_sun_direction,
          w * contact_shadow_len*fovScale, shadow_steps, dither - 0.5, projectionMatrix,
          w, viewProjectionMatrixNoOfs, hitUV, float2(100, 99)
        );
        gtao_value.CONTACT_SHADOWS_ATTR = getAdditionalShadow(gtao_value.CONTACT_SHADOWS_ATTR, cameraToPoint, wsNormal);
      }
      ##endif


      #ifdef GBUFFER_HAS_HERO_COCKPIT_BIT
        BRANCH
        if (isGbufferHeroCockpit(texcoord))
        {
          float fade = saturate(dot(float4(pixelPos, 1), hero_cockpit_vec));
          GTAO_VALUE_X = lerp(GTAO_VALUE_X, 1.0f, fade);
          ##if maybe(ssao_contact_shadows)
            gtao_value.CONTACT_SHADOWS_ATTR = lerp(gtao_value.CONTACT_SHADOWS_ATTR, 1.0f, fade);
          ##endif
        }
      #endif

      ##if maybe(ssao_wsao)
        gtao_value.WSAO_ATTR = getWsao(cameraToPoint + world_view_pos, wsNormal);
      ##endif
      return setSSAO(gtao_value);
    }
  }
endmacro

shader gtao_main
{
  z_write = false;
  z_test = false;
  cull_mode = none;
  color_write = rgb;

  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  GTAO_MAIN_CORE(ps)

  hlsl(ps) {
    SSAO_TYPE gtao_resolve_ps(VsOutput input HW_USE_SCREEN_POS) : SV_Target0
    {
      return gtao_resolve(GET_SCREEN_POS(input.pos).xy, input.texcoord.xy, input.viewVect.xyz);
    }
  }
  compile("target_ps", "gtao_resolve_ps");
}

shader gtao_main_cs
{
  hlsl {
    #define NO_GRADIENTS_IN_SHADER 1
  }

  USE_AND_INIT_VIEW_VEC_CS()
  GTAO_MAIN_CORE(cs)

  hlsl(cs) {
    RWTexture2D<float3> gtao_target : register(u0);

    [numthreads( 8, 8, 1 )]
    void gtao_resolve_cs(uint3 DTid : SV_DispatchThreadID)
    {
      if (any(DTid.xy >= gtao_texel_size.zw))
        return;

      float2 pixelCenter = DTid.xy + 0.5;
      float2 texcoord    = pixelCenter * gtao_texel_size.xy;
      float3 worldDir    = lerp_view_vec(texcoord);
      SSAO_TYPE ao = gtao_resolve(pixelCenter, texcoord, worldDir);
      gtao_target[DTid.xy] = ao;
    }
  }
  compile("target_cs", "gtao_resolve_cs");
}

macro GTAO_SPATIAL_CORE(code)
  supports none;

  INIT_ZNZFAR_STAGE(code)
  INIT_HALF_RES_DEPTH(code)
  INIT_VIEW_VEC_STAGE(code)
  USE_VIEW_VEC_STAGE(code)

  local float4 halfResTexSize = get_dimensions(raw_gtao_tex, 0);
  (code) {
    raw_gtao_tex@smp2d = raw_gtao_tex;
    gtao_texel_size@f4 = (1.0 / halfResTexSize.xy, halfResTexSize.xy);
  }

  hlsl(code) {

    SSAO_TYPE gtao_spatial(float2 texcoord)
    {
      // Get the depth of the "center" sample - this reference depth is used to weight the other samples
      float rawDepth = tex2Dlod(half_res_depth_tex, float4(texcoord, 0, 0)).r;

      BRANCH
      if (rawDepth >= 1)
        return 1;

      float depth = decode_depth(rawDepth, zn_zfar.zw);
      float weightsSpatial = 0.0;
      SSAO_TYPE ao = 0.0;

      UNROLL
      for (int y = 0; y < 4; y++)
      {
        UNROLL
        for (int x = 0; x < 4; x++)
        {
          // Weight each sample by its distance from the refrence depth - but also scale the weight by 1/10 of the
          // reference depth so that the further from the camera the samples are, the higher the tolerance for depth
          // differences is.
          float2 uv = texcoord + float2(x - 2, y - 2) * gtao_texel_size.xy;
          float localDepth = decode_depth(tex2Dlod(half_res_depth_tex, float4(uv, 0, 0)).r, zn_zfar.zw);
          float localWeight = max(0.0, 1.0 - abs(localDepth - depth) / (depth * 0.1));
          weightsSpatial += localWeight;
          ao += tex2Dlod(raw_gtao_tex, float4(uv, 0, 0)).SSAO_ATTRS * localWeight;
        }
      }
      ao /= weightsSpatial;

      return ao;
    }
  }
endmacro

shader gtao_spatial
{
  z_write = false;
  z_test = false;
  cull_mode = none;
  color_write = rgb;

  POSTFX_VS_TEXCOORD(0, texcoord)
  GTAO_SPATIAL_CORE(ps)

  hlsl(ps) {

    SSAO_TYPE gtao_spatial_ps(VsOutput input) : SV_Target0
    {
      return gtao_spatial(input.texcoord);
    }
  }

  compile("target_ps", "gtao_spatial_ps");
}

shader gtao_spatial_cs
{
  GTAO_SPATIAL_CORE(cs)

  hlsl(cs) {
    RWTexture2D<float3> gtao_target : register(u0);

    [numthreads( 8, 8, 1 )]
    void gtao_spatial_cs(uint3 DTid : SV_DispatchThreadID)
    {
      if (any(DTid.xy >= gtao_texel_size.zw))
        return;

      float2 pixelCenter = DTid.xy + 0.5;
      float2 texcoord    = pixelCenter * gtao_texel_size.xy;
      SSAO_TYPE ao = gtao_spatial(texcoord);
      gtao_target[DTid.xy] = ao;
    }
  }

  compile("target_cs", "gtao_spatial_cs");
}

macro GTAO_TEMPORAL_CORE(code)
  supports none;

  INIT_ZNZFAR_STAGE(code)
  SSAO_REPROJECTION(code, gtao_prev_tex)
  INIT_HALF_RES_DEPTH(code)
  INIT_VIEW_VEC_STAGE(code)
  USE_VIEW_VEC_STAGE(code)

  local float4 halfResTexSize = get_dimensions(gtao_tex, 0);
  (code) {
    gtao_tex@smp2d = gtao_tex;
    gtao_texel_size@f4 = (1.0 / halfResTexSize.xy, halfResTexSize.xy);
  }

  hlsl(code) {

    SSAO_TYPE gtao_temporal(float2 texcoord)
    {
      // Reconstruct position from depth
      // Note that the position is relative to the camera position (not an absolute world space position)
      float3 viewVec = lerp_view_vec(texcoord);
      float rawDepth = tex2Dlod(half_res_depth_tex, float4(texcoord, 0, 0)).r;

      BRANCH
      if (rawDepth >= 1)
        return 1;

      float depth = decode_depth(rawDepth, zn_zfar.zw);
      float3 cameraToPoint = viewVec * depth;

      SSAO_TYPE ao = tex2Dlod(gtao_tex, float4(texcoord, 0, 0)).SSAO_ATTRS;

      // Get neighborhood
      SSAO_TYPE minAo = ao, maxAo = ao;
      UNROLL for (int y = -1; y <= 1; y++)
      {
        UNROLL for (int x = -1; x <= 1; x++)
        {
          float2 uv = texcoord + float2(x, y) * gtao_texel_size.xy;

          // Sample neighbor AO
          SSAO_TYPE neighborAo = tex2Dlod(gtao_tex, float4(uv, 0, 0)).SSAO_ATTRS;

          // Blend neighbor AO to center AO if neighbor depth is too different from center depth
          float neighborDepth = decode_depth(tex2Dlod(half_res_depth_tex, float4(uv, 0, 0)).r, zn_zfar.zw);
          float neighborWeight = max(0.0, 1.0 - abs(neighborDepth - depth) / (depth * 0.1));
          neighborAo = lerp(ao, neighborAo, neighborWeight);

          // Adjust min and max values found in neighborhood
          minAo = min(minAo, neighborAo);
          maxAo = max(maxAo, neighborAo);
        }
      }

      reproject_ssao_with_neighborhood(ao, minAo, maxAo, cameraToPoint, texcoord, depth);

      return ao;
    }
  }
endmacro

shader gtao_temporal
{
  z_write = false;
  z_test = false;
  cull_mode = none;
  color_write = rgb;

  POSTFX_VS_TEXCOORD(0, texcoord)
  GTAO_TEMPORAL_CORE(ps)

  hlsl(ps) {

    SSAO_TYPE gtao_temporal_ps(VsOutput input) : SV_Target0
    {
      return gtao_temporal(input.texcoord);
    }
  }

  compile("target_ps", "gtao_temporal_ps");
}

shader gtao_temporal_cs
{
  GTAO_TEMPORAL_CORE(cs)

  hlsl(cs) {
    RWTexture2D<float3> gtao_target : register(u0);

    [numthreads( 8, 8, 1 )]
    void gtao_temporal_cs(uint3 DTid : SV_DispatchThreadID)
    {
      if (any(DTid.xy >= gtao_texel_size.zw))
        return;

      float2 pixelCenter = DTid.xy + 0.5;
      float2 texcoord    = pixelCenter * gtao_texel_size.xy;
      SSAO_TYPE ao = gtao_temporal(texcoord);
      gtao_target[DTid.xy] = ao;
    }
  }

  compile("target_cs", "gtao_temporal_cs");
}
