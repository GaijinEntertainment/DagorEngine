hlsl
{
  #define pfx_lib_pi 3.14159265359
  #define pfx_lib_2pi ( pfx_lib_pi * 2.f )
}

macro PFX_LIB_RND()
  hlsl
  {
    Texture2D rnd_tex:register(t7);

    #define pfx_lib_rnd_tex_width 128
    #define pfx_lib_rnd_tex_height 32

    float pfx_lib_tex_srnd( in out uint seed )
    {
      float v = texelFetch( rnd_tex, int2( seed % pfx_lib_rnd_tex_width, ( seed / pfx_lib_rnd_tex_width ) % pfx_lib_rnd_tex_height ), 0 ).x;
      seed++;
      return v;
    }

    float pfx_lib_tex_rnd( in out uint seed )
    {
      return pfx_lib_tex_srnd( seed ) * 0.5 + 0.5;
    }

    float3 pfx_lib_tex_srnd_vec3( in out uint seed )
    {
      float r0 = pfx_lib_tex_srnd( seed );
      float r1 = pfx_lib_tex_srnd( seed );
      float r2 = pfx_lib_tex_srnd( seed );

      return float3( r0, r1, r2 );
    }

    float3 pfx_lib_tex_rnd_vec3( in out uint seed )
    {
      return pfx_lib_tex_srnd_vec3( seed ) * 0.5 + 0.5;
    }
  }
endmacro

macro PFX_LIB_INTERSECT()
  hlsl
  {
    float3 ray_intersect_sphere_dist( float3 ray_start,
                                     float3 ray_dir_norm,
                                     float3 sphere_center,
                                     float sphere_radius )
    {
      float3 dir = ray_dir_norm;

      float3 p = ray_start - sphere_center;

      float b = 2.f * dot( p, dir );
      float c = dot( p, p ) - sphere_radius * sphere_radius;
      float d = b * b - 4 * c;
      if ( d >= 0 )
      {
        float sq = sqrt( d );
        float v0 = ( -b - sq ) * 0.5f;
        float v1 = ( -b + sq ) * 0.5f;

        return float3( v0 >= 0.f ? v0 : max( v0, v1 ), v0, v1 );
      }

      return -1.f;
    }
  }
endmacro

macro PFX_LIB_SHAPES()
  PFX_LIB_RND()
  hlsl
  {
    float3 pfx_lib_shape_solid_sphere_rnd( in out uint seed )
    {
      float r0 = pfx_lib_tex_srnd( seed );
      float r1 = pfx_lib_tex_srnd( seed );
      float r2 = pfx_lib_tex_srnd( seed );

      return float3( r0, r1, r2 );
    }
  }
endmacro