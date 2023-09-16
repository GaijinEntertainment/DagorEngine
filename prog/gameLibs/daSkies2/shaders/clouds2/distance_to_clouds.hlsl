#ifndef DISTANCE_TO_CLOUDS_HLSL_INCLUDED
#define DISTANCE_TO_CLOUDS_HLSL_INCLUDED 1

void calc_above_clouds(float2 distInner, float2 distOuter, out float dist0, out float dist1)
{
  //if we are above clouds, we need compute dist0 as distance to top_layer and dist1 as second distance to top_layer
  dist0 = distOuter.x;//min(dist.w, dist.y);
  dist1 = distInner.x > 0 ? distInner.x : distOuter.y;//max(dist.w, dist.y);
}

//if below clouds, then dist0 is dist to bottom, and dist1 is distance to top
void calc_below_clouds(float2 distInner, float2 distOuter, out float dist0, out float dist1)
{
  dist0 = distInner.y;
  dist1 = distOuter.y;
}

//if in clouds, then dist0 = 0, and dist1 is distance to wither top or bottom (whichever closer
void calc_in_clouds(float2 distInner, float2 distOuter, out float dist0, out float dist1)
{
  //if we are inside clouds, we don't need dist0
  dist0 = 0;
  //dist1 = b + sqrtDscr.y;
  dist1 = (distInner.x > 0 && distOuter.y > (2*INFINITE_TRACE_DIST/1000)) ? distInner.x : distOuter.y;
}

void math_distance_to_clouds(float mu, out float dist0, out float dist1, float radius_to_point, float radius_to_point_sq, float4 clouds_layer_info)
{

  float Rh=radius_to_point;//radius+world_view_pos.y/1000;//radius to camera
  float b=Rh*mu;
  clouds_layer_info.zw += radius_to_point_sq;

  float2 b24c=(b*b).xx - clouds_layer_info.zw;
  float2 sqrtDscr = sqrt(b24c);
  float2 distInner = b24c.x < 0 ? -1 : b.xx + float2(-sqrtDscr.x, sqrtDscr.x);
  float2 distOuter = b24c.y < 0 ? -1 : b.xx + float2(-sqrtDscr.y, sqrtDscr.y);

  if (Rh <= clouds_layer_info.x)
    calc_below_clouds(distInner, distOuter, dist0, dist1);
  else if (Rh <= clouds_layer_info.y)
    calc_in_clouds(distInner, distOuter, dist0, dist1);
  else
    calc_above_clouds(distInner, distOuter, dist0, dist1);
}

#endif