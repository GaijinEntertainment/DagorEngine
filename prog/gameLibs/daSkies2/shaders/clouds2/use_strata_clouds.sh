texture strata_clouds;

float strata_tc_scale = 0.00005;
float strata_pos_x = 0;
float strata_pos_z = 0;

macro USE_STRATA_CLOUDS(code)
  local float radius_to_camera_offseted = (skies_planet_radius+(skies_world_view_pos.y)/1000);
  (code) {
    planet_radius_offseted@f3 = (radius_to_camera_offseted*radius_to_camera_offseted,
      radius_to_camera_offseted*radius_to_camera_offseted-skies_planet_radius*skies_planet_radius,
      radius_to_camera_offseted,0);
  }
  (code) {
    strata_clouds@smp2d = strata_clouds;
    strata_tc_scale0@f4 = (strata_tc_scale, strata_tc_scale, 0.5 + (skies_world_view_pos.x - strata_pos_x)*strata_tc_scale, 0.5 + (skies_world_view_pos.z - strata_pos_z)*strata_tc_scale);

    strata_clouds_pow@f4 = (1 / (0.00001 + strata_clouds_effect), strata_clouds_altitude, strata_clouds_altitude + skies_planet_radius,
      (strata_clouds_altitude + skies_planet_radius)*(strata_clouds_altitude + skies_planet_radius));
    strata_clouds_effect@f1 = (strata_clouds_effect)
  }
  hlsl(code) {
    half henyey_greenstein(half g, half cos0)//move to common
    {
      half g2 = g * g;
      half h = 1 + g2 - 2 * g * cos0;
      return (1./(4.*PI)) * (1 - g2) * pow(h, -3./2);
      //return (0.5 * 0.079577) + (0.5/(4 * PI)) * (1 - g2) * rcp(h * h * rsqrt(h));
    }

    half4 get_strata_clouds(float3 view, float2 scattering_tc)
    {
      //float distanceToClouds0,distanceToClouds1;
      //distance_to_clouds(point2EyeNrm, distanceToClouds0, distanceToClouds1);
      float r = theAtmosphere.bottom_radius + skies_world_view_pos.y/1000;//bruneton_camera_height.x;
      float dscr = planet_radius_offseted.x * (view.y * view.y - 1.0) + strata_clouds_pow.w;
      if (dscr < 0)
        return 0;
      dscr = sqrt(dscr);
      float ry = -r * view.y;
      //float distanceToClouds = ry + (ry > dscr ? -dscr : +dscr);
      //clip(distanceToClouds);
      float distanceToClouds = ry + dscr;
      float dist = distanceToClouds*1000;

      //==
      //return float4(0,0,0, 1-tex3Dlod(clouds, float4(texcoord,0.125,0) ).w);
      //fixme:
      float distToGround = dist+1000;
      if (view.y<0)
      {

        //==fixme: remove
        float Rh=planet_radius_offseted.z;
        float b=Rh*view.y;
        float c=planet_radius_offseted.y;//Rh*Rh-RH*RH
        float b24c=b*b-c;
        float distToGround = -b-sqrt(b24c);
        if (b24c >= 0 && distToGround>=0 && distToGround < distanceToClouds)
          return 0;
      }

      half cos0 = dot(skies_primary_sun_light_dir.xyz, view.xzy);

      float2 stratatc = view.xz*(dist*strata_tc_scale0.x)+strata_tc_scale0.zw;
      #ifdef USE_STRATA_LOD
      half first_density = tex2Dlod(strata_clouds, float4(stratatc,0,USE_STRATA_LOD(stratatc))).x;
      #else
      half first_density = tex2D(strata_clouds, stratatc).x;
      #endif
      //float threshold = lerp(0.3, 0.2, strata_clouds_effect);
      //first_density = saturate((first_density-threshold)/(1-threshold));
      first_density = pow(first_density, 2.4);//gamma like
      half second_density = tex2D(strata_clouds, float2(0.31,0.352)*stratatc+float2(0.171,0.131)).x;
      //second_density = saturate((second_density-0.25)/(1-0.25));
      second_density = pow(second_density, 1.2*strata_clouds_pow.x);
      half cloud_density = saturate(first_density*smoothstep(0,1,second_density));
      half initial_density = cloud_density;

      //cloud_density *= cloud_density;
      //cloud_density = pow(cloud_density, strata_clouds_pow.x);
      float2 scatteringTC;
      scatteringTC = long_get_prepared_scattering_tc(view.y, dist, preparedScatteringDistToTc);
      cloud_density *= (saturate(2-2*scatteringTC.x));
      //cloud_density *= pow2(saturate(1-dist*(1/MAX_CLOUDS_DIST)));//disable strata clouds in 280km
      //cloud_density *= saturate(skies_world_view_pos.y-);//disable strata clouds in 50m
      //fixme:
      cloud_density *= saturate(smoothstep(strata_clouds_pow.y*1000, strata_clouds_pow.y*900, skies_world_view_pos.y));//constant!!!!
      //fixme:
      //todo - do not render at all if above clouds
      if (cloud_density<=0.0001)
        return 0;
      float3 sun_color, sky_color;
      get_sun_sky_light(view*distanceToClouds*1000 + skies_world_view_pos.xyz, sun_color, sky_color);

      float strata_clouds_light_extinction = -4;
      float beers_term = exp2(cloud_density * strata_clouds_light_extinction);
      float betaCoef = 0.9;//strata_clouds_eccentricity;//*(1-cloud_density);
      float betaCoef2 = -0.4;//*(1-cloud_density);
      half3 luminance =
            (lerp(henyey_greenstein(betaCoef, cos0), henyey_greenstein(betaCoef2, cos0), 0.4)) * sun_color
          + (sky_color)/2;
      half3 color = luminance;
    ##if mobile_render == off
      apply_scattering_tc_fog(scattering_tc, view, dist, color);
    ##endif
      return half4(color, 1-beers_term);
    }
  }
endmacro

