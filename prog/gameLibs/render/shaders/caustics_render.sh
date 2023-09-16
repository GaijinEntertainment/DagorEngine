include "shader_global.sh"
include "gbuffer.sh"
include "viewVecVS.sh"
include "caustics.sh"
include "static_shadow.sh"
include "frustum.sh"
include "water_heightmap.sh"

texture downsampled_normals;
float4 caustics_texture_size;
texture water_heightmap_lowres;
texture downsampled_close_depth_tex;

macro CAUSTICS_RENDER_CORE(code)
  INIT_ZNZFAR_STAGE(code)

  (code) {
    world_view_pos@f3 = world_view_pos;
    downsampled_close_depth_tex@smp2d = downsampled_close_depth_tex;

    downsampled_normals@smp2d = downsampled_normals;

    inv_caustics_texture_size@f2 = (1.0/caustics_texture_size.x, 1.0/caustics_texture_size.y);
  }
  if (water_heightmap_lowres != NULL)
  {
    INIT_WORLD_TO_WATER_HEIGHTMAP(code)
    (code) {water_heightmap_lowres@smp2d = water_heightmap_lowres;}
  }
  else
  {
    (code) {water_level@f1 = water_level;}
  }

  if (static_shadows_cascades != off)
  {
    hlsl(code) {
      #define HAS_STATIC_SHADOW 1
    }

    INIT_STATIC_SHADOW_BASE(code)
    USE_STATIC_SHADOW_BASE(code)
    hlsl(code){
      #define get_shadow(shadowPos) getStaticShadow( shadowPos.xyz )
    }
  }
  else
  {
    hlsl(code){
      #define get_shadow(shadowPos) 1
    }
  }

  hlsl(code) {
    #define USE_CAUSTICS_RENDER 1
  }

  USE_SSR_CAUSTICS(code)

  hlsl(code) {
    half4 caustics(float2 texcoord, float3 viewVect)
    {
      float rawDepth = tex2Dlod(downsampled_close_depth_tex, float4(texcoord, 0, 0)).x;
      float w = linearize_z(rawDepth, zn_zfar.zw);

      BRANCH
      if (rawDepth <= 0)
        return 0;

      half3 normal;
      half4 normal_smoothness = tex2Dlod(downsampled_normals, float4(texcoord,0,0));
      normal = normal_smoothness.xyz*2-1;

      float3 cameraToPoint = viewVect.xyz * w;
      float3 worldPos = world_view_pos + cameraToPoint;
      float worldDist=length(cameraToPoint);

      ##if water_heightmap_lowres != NULL
      float2 water_heightmap_lowres_uv = worldPos.xz * world_to_water_heightmap.zw + world_to_water_heightmap.xy;
      float water_level = tex2Dlod(water_heightmap_lowres, float4(water_heightmap_lowres_uv, 0, 0)).r;
      ##endif

      half4 result = get_ssr_caustics(worldPos, normal, water_level, worldDist);

      return sqrt(result);
    }
  }
endmacro

shader caustics_render
{
  no_ablend;
  cull_mode = none;
  z_write = false;
  z_test = false;

  CAUSTICS_RENDER_CORE(ps)

  USE_AND_INIT_VIEW_VEC_VS()
  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  hlsl(ps) {
    half4 caustics_ps(VsOutput input) : SV_Target
    {
      return caustics(input.texcoord, input.viewVect);
    }
  }
  compile("target_ps", "caustics_ps");
}

shader caustics_render_cs
{
  if (hardware.fsh_5_0)
  {
    USE_AND_INIT_VIEW_VEC_CS()
    CAUSTICS_RENDER_CORE(cs)

    hlsl(cs) {
      RWTexture2D<float4> caustics_tex : register(u0);

      [numthreads(8, 8, 1)]
      void caustics_cs(uint3 DTid : SV_DispatchThreadID)
      {
        float2 pixelCenter = DTid.xy + 0.5;
        float2 texcoord    = pixelCenter * inv_caustics_texture_size.xy;
        float3 viewVect    = lerp_view_vec(texcoord);

        caustics_tex[DTid.xy] = caustics(texcoord, viewVect);
      }
    }
    compile("target_cs", "caustics_cs");
  }
  else
  {
    dont_render;
  }
}

shader indoor_probe_to_depth
{
  supports global_frame;
  supports none;
  color_write = 0;
  z_test = true;
  z_write = true;
  cull_mode = none;
  channel float3 pos = pos;
  (vs) {globtm@f44 = globtm;}
  hlsl(vs)
  {
    float4 very_basic_vs(float3 pos : POSITION) : SV_POSITION
    {
      return mulPointTm(pos, globtm);
    }
  }
  compile("target_vs", "very_basic_vs");
  compile("ps_null", "null_ps");
}