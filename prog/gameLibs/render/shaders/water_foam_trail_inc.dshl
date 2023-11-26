texture water_foam_trail;
texture water_foam_trail_mask;
float water_foam_trail_cascade_count = 4;
float4 water_foam_blend_params = (1, 0.25, 0, 0);
float4 water_foam_trail_mat00 = (1, 0, 0, 0);
float4 water_foam_trail_mat01 = (0, 1, 0, 0);
float4 water_foam_trail_mat10 = (1, 0, 0, 0);
float4 water_foam_trail_mat11 = (0, 1, 0, 0);
float4 water_foam_trail_mat20 = (1, 0, 0, 0);
float4 water_foam_trail_mat21 = (0, 1, 0, 0);
float4 water_foam_trail_mat30 = (1, 0, 0, 0);
float4 water_foam_trail_mat31 = (0, 1, 0, 0);

macro INIT_WATER_FOAM_TRAIL()
  if ( support_texture_array == on && water_foam_trail != NULL )
  {
    (ps) {
      water_foam_trail@smpArray = water_foam_trail;
      water_foam_trail_mat00@f4= water_foam_trail_mat00;
      water_foam_trail_mat01@f4= water_foam_trail_mat01;
      water_foam_trail_mat10@f4= water_foam_trail_mat10;
      water_foam_trail_mat11@f4= water_foam_trail_mat11;
      water_foam_trail_mat20@f4= water_foam_trail_mat20;
      water_foam_trail_mat21@f4= water_foam_trail_mat21;
      water_foam_trail_mat30@f4= water_foam_trail_mat30;
      water_foam_trail_mat31@f4= water_foam_trail_mat31;
      water_foam_trail_cascade_count@f1 = (water_foam_trail_cascade_count);
      water_foam_blend_params@f2 = water_foam_blend_params;
    }

    hlsl
    {
      #define WATER_FOAM_TRAIL
    }
  }
endmacro

macro USE_MODULATED_FOAM()
  hlsl(ps)
  {
    float get_modulated_foam( float v, float base_foam )
    {
      //const float threshold_part = 1.3;
      //return saturate(threshold_part * v) * saturate(base_foam - (1.0 / threshold_part) * (1.0 - v));

      const float blend = 0.5;
      const float boost = 0.2;
      float masked = saturate((base_foam + lerp(-1, boost, v)) / (blend * v + 1 - blend)) * base_foam;
      return masked;
    }

    float get_wake_foam( float2 grad, float2 dir, float base_foam )
    {
      float front_k = pow4( saturate( dot( grad.xy * 0.15f, dir.xy ) ) );
      float back_k = pow4( saturate( dot( -grad.xy * 0.1f, dir.xy ) ) );
      float side_k = pow4( saturate( abs( dot( float2( grad.y, -grad.x ) * 0.05f, dir.xy ) ) ) );

      return
        get_modulated_foam( front_k, base_foam ) * 0.75 +
        get_modulated_foam( back_k, base_foam ) * 0.25f +
        get_modulated_foam( side_k, base_foam ) * 0.25f;
    }
  }
endmacro

macro USE_WATER_FOAM_TRAIL()
  hlsl(ps)
  {
#ifdef WATER_FOAM_TRAIL

    float3 normalize_trail_tc( float3 tc )
    {
      tc.xy *= 0.5;
      tc.x = 0.5f + tc.x;
      tc.y = 0.5f - tc.y;

      return tc;
    }

    float3 get_water_foam_trail_tc( float3 world_pos )
    {
      float4 wp = float4( world_pos.x, 0.f, world_pos.z, 1.f );

      float3 t0;
      t0.x = dot( wp, water_foam_trail_mat00 );
      t0.y = dot( wp, water_foam_trail_mat01 );
      t0.z = 0;

      float3 t1;
      t1.x = dot( wp, water_foam_trail_mat10 );
      t1.y = dot( wp, water_foam_trail_mat11 );
      t1.z = 1;

      float3 t2;
      t2.x = dot( wp, water_foam_trail_mat20 );
      t2.y = dot( wp, water_foam_trail_mat21 );
      t2.z = 2;

      float3 t3;
      t3.x = dot( wp, water_foam_trail_mat30 );
      t3.y = dot( wp, water_foam_trail_mat31 );
      t3.z = 3;

      float3 res = float3( 0.f, 0.f, -1.f );
      res = all( abs( t3.xy ) < float2( 1.f, 1.f ) ) ? t3 : res;
      res = all( abs( t2.xy ) < float2( 1.f, 1.f ) ) ? t2 : res;
      res = all( abs( t1.xy ) < float2( 1.f, 1.f ) ) ? t1 : res;
      res = all( abs( t0.xy ) < float2( 1.f, 1.f ) ) ? t0 : res;
      return res;
    }

    float get_water_foam_vignette( float3 local_tc )
    {
      float2 v = saturate( abs( local_tc.xy * 2.f - 1.f ) * 8.f - 7.f );
      float lastCascade = local_tc.z > water_foam_trail_cascade_count - 1.5f ? 1.f : 0.f;
      return 1.f - saturate( dot( v, v ) ) * lastCascade;
    }

    void get_water_foam_trail( float3 world_pos, float base_foam, float3 grad, inout float out_inscatter, inout float out_foam )
    {
      float grlen = length( grad.xy );
      grad.xy *= grlen > 0 ? rcp( grlen ) : 0;
      world_pos.xz += grad.xy * 0.15f;
      float3 local_tc = get_water_foam_trail_tc( world_pos );

      local_tc = normalize_trail_tc( local_tc );
      if ( local_tc.z < 0.f )
        return;

      float res = tex3D( water_foam_trail, local_tc ).x;
      res *= get_water_foam_vignette( local_tc );

      out_foam += get_modulated_foam( res.x, base_foam ) * water_foam_blend_params.x;
      out_inscatter += res * pow4( 1.f - base_foam ) * water_foam_blend_params.y;
    }

    void get_water_foam_trail_comp( float3 world_pos, inout float out_foam )
    {
      float3 local_tc = get_water_foam_trail_tc( world_pos );

      local_tc = normalize_trail_tc( local_tc );
      if ( local_tc.z < 0.f )
        return;

      float res = tex3D( water_foam_trail, local_tc ).x;
      res *= get_water_foam_vignette( local_tc );
      out_foam += saturate( res.x + 0.5f ) * res.x * 0.5f;
    }

#else
    void get_water_foam_trail( float3 world_pos, float base_foam, float grad, inout float out_inscatter, inout float out_foam )
    {
    }
#endif
  }
endmacro