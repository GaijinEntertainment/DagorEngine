#ifndef STATISTICAL_CLOUDS_SHADOW_HLSLI
#define STATISTICAL_CLOUDS_SHADOW_HLSLI 1
  float getCloudsLength(float r, float mu, float clouds_bottom_radius_sq, float clouds_top_radius_sq)
  {
    float rmu = r * mu;
    float rSq = r*r;
    float rmuSq = (rmu*rmu - rSq);
    float2 discriminant_bottom_top = float2(rmuSq + clouds_bottom_radius_sq, rmuSq + clouds_top_radius_sq);
    float2 sqDiscr = sqrt(max(float2(0,0), discriminant_bottom_top));
    float2 dist_to_bottom = discriminant_bottom_top.x < 0 ? float2(0,0) : max(float2(0,0), float2(-rmu, -rmu) + float2(-sqDiscr.x, sqDiscr.x));
    float2 dist_to_top = discriminant_bottom_top.y < 0 ? float2(0,0) : max(float2(0,0), float2(-rmu, -rmu) + float2(-sqDiscr.y, sqDiscr.y));
    float len = max(0.f, max(dist_to_top.x, dist_to_top.y) - max(dist_to_bottom.x, dist_to_bottom.y));
    len += max(0.f, min(dist_to_bottom.x, dist_to_bottom.y) - min(dist_to_top.x, dist_to_top.y));
    return len;
  }
#endif