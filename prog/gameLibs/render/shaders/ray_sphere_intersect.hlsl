#ifndef RAY_SPHERE_INTERSECT
#define RAY_SPHERE_INTERSECT 1

bool ray_intersect_sphere( float3 ray_start,
                           float3 ray_dir_norm,
                           float3 sphere_center,
                           float sphere_radius,
                           out float3 out_pos )
{
  float3 dir = ray_dir_norm;

  float3 p = ray_start - sphere_center;

  float b = 2.f * dot( p, dir );
  float c = dot( p, p ) - sphere_radius * sphere_radius;
  float d = b * b - 4 * c;

  FLATTEN
  if ( d >= 0 )
  {
    float sq = sqrt( d );
    float v0 = ( -b - sq ) * 0.5f;
    float v1 = ( -b + sq ) * 0.5f;

    out_pos = float3( v0 >= 0.f ? v0 : max( v0, v1 ), v0, v1 );
    return true;
  }
  else
  {
    out_pos = ray_start;
    return false;
  }
}

#endif