float4 skies_moon_dir;
float4 skies_moon_color;
texture preparedMoonInscatter;
int skies_moon_additional = 0;
interval skies_moon_additional : off<1, on;

macro INIT_AND_USE_MOON()
  if (skies_moon_additional == on)
  {
    (ps) {
      skies_moon_dir@f3 = skies_moon_dir;
      skies_moon_color@f3 = skies_moon_color;
      preparedMoonInscatter@smp2d = preparedMoonInscatter;
    }

    hlsl(ps) {
      #define moonDir (skies_moon_dir.xyz)
      #define IMoon (skies_moon_color)
    }
  }
endmacro


//visible moon
float4 stars_moon_dir;
float stars_moon_size;
float stars_moon_intensity;
texture stars_moon_tex;
macro INIT_MOON_TEX()
  local float moon_radius_norm = sin(stars_moon_size)/cos(stars_moon_size);
  (ps) {
    stars_moon_dir@f4 = (stars_moon_dir.x, stars_moon_dir.y, stars_moon_dir.z, 1./stars_moon_size);
    moon_age_sincos@f4 = (sin((2*PI)*stars_moon_dir.w+PI),
                          cos((2*PI)*stars_moon_dir.w+PI),
                          cos(stars_moon_size), stars_moon_intensity);
    auto_phase@f2 = (moon_radius_norm*moon_radius_norm-1, 1./moon_radius_norm, 0, 0);
    moon_real_sun_light_dir@f3 = real_skies_sun_light_dir;
    stars_moon_tex@smp2d = stars_moon_tex;
  }

  hlsl(ps) {
    #define SAMPLE_MOON_TEX 1

    float raySphereIntersect(float moon_nu, float sphere_radius2)
    {
      float disc = max(0, moon_nu * moon_nu - 1 + sphere_radius2);
      return moon_nu - sqrt(disc);
    }
    half4 getMoonTex(float3 v)
    {
      float3 moon_dir = stars_moon_dir.xzy;
      float moon_nu = dot(moon_dir, v);
      BRANCH
      if (moon_nu>moon_age_sincos.z)
      {
        float3 xcol = cross(float3(0,1,0), moon_dir), ycol;
        float xAxisLength = dot(xcol,xcol);
        if (xAxisLength <= 1.192092896e-06F)
        {
          xcol = normalize(cross(float3(0, 0, moon_dir.y/abs(moon_dir.y)), moon_dir));
          ycol = normalize(cross(moon_dir, xcol));
        } else
        {
          xcol *= rsqrt(xAxisLength);
          ycol = normalize(cross(moon_dir, xcol));
        }
        float3x3 tm = float3x3(xcol, ycol, moon_dir);
        float2 dir = mul(tm, v).yx*stars_moon_dir.w;
        //dir.x = -dir.x;
        half4 moonTex = tex2Dlod(stars_moon_tex, float4(-0.5*dir.yx+0.5,0,0));

        //float moon_radius_norm = tan(1./stars_moon_dir.w);
        //float moonHitPos = raySphereIntersect(moon_nu, moon_radius_norm*moon_radius_norm);
        float moonHitPos = moon_nu - sqrt(max(0, moon_nu * moon_nu + auto_phase.x));
        float3 moonPos = moonHitPos*v;
        float3 moon_center = moon_dir;
        float3 moonNorm = (moonPos - moon_center)*auto_phase.y;//normalization

        moonTex.rgb *= saturate(dot(moonNorm, moon_real_sun_light_dir.xyz)*3);//simplified shading. Moon is very rough, and so Oren-Nayar or something, which basically results in hard fall off

        return half4(moonTex.rgb, moonTex.a*moon_age_sincos.w);//*moon_color
      }
      return 0;
    }
    
  }
endmacro
