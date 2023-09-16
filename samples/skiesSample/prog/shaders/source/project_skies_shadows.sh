include "shader_global.sh"

texture hmap_ldetail;
float4 world_to_hmap_low = (1,1,1,1);
float4 heightmap_scale = (20,0,0,0);
int enable_god_rays_from_land;

macro PROJECT_SKIES_SHADOWS(code)
  (code) {
    hmap_ldetail@smp2d = hmap_ldetail;
    world_to_hmap_mul@f3 = (world_to_hmap_low.x,1./heightmap_scale.x,world_to_hmap_low.y,0);
    world_to_hmap_add@f4 = (world_to_hmap_low.z,-heightmap_scale.y/heightmap_scale.x,world_to_hmap_low.w,enable_god_rays_from_land);
  }
  hlsl(code) {
    float get_project_skies_shadows(float3 worldPos, float dist_in_km, float radius_in_km, float view_mu)
    {
      if (!world_to_hmap_add.w)
        return 1;//commented out, because slow
      float stepSize = 32.f;
      worldPos.y += 4.0;
      float3 rayPos = worldPos*world_to_hmap_mul + world_to_hmap_add.xyz;
      float3 rayStep = skies_primary_sun_light_dir.xzy*world_to_hmap_mul*stepSize;
      uint steps = 64;
      for (uint i = 0; i < steps; ++i, rayStep*=1.25)
      {
        rayPos += rayStep;
        float height = tex2Dlod(hmap_ldetail, float4(rayPos.xz,0,0)).x;
        if (rayPos.y < height)
          return 0;
      }
      return 1;
    }
  }
endmacro
