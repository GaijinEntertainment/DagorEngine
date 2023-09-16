float4 fom_shadows_tm_x;
float4 fom_shadows_tm_y;
float4 fom_shadows_tm_z;
texture fom_shadows_cos;
texture fom_shadows_sin;

int assume_fom_shadows;
interval assume_fom_shadows:off<1, on;

macro INIT_FOM_SHADOWS(stage)
  if (assume_fom_shadows == on)
  {
    (stage)
    {
      fom_shadows_tm_x@f4 = fom_shadows_tm_x;
      fom_shadows_tm_y@f4 = fom_shadows_tm_y;
      fom_shadows_tm_z@f4 = fom_shadows_tm_z;
      fom_shadows_cos@smp2d = fom_shadows_cos;
      fom_shadows_sin@smp2d = fom_shadows_sin;
    }
  }
endmacro

macro USE_FOM_SHADOWS(stage)
  hlsl(stage) {
    ##if (assume_fom_shadows == on)
    float getFOMShadow(float3 worldPos)
    {
      BRANCH
      if (fom_shadows_tm_z.w<-1000000)//uniform branch for early out, if nothing renered to fom
        return 1;
      float3 tc = float3(dot(float4(worldPos, 1), fom_shadows_tm_x), dot(float4(worldPos, 1), fom_shadows_tm_y), dot(float4(worldPos, 1), fom_shadows_tm_z));
      float4 sR_a0123 = tex2Dlod(fom_shadows_cos, float4(tc.xy, 0, 0));
      //return 1-max(max(sR_a0123.r, sR_a0123.g), sR_a0123.b);
      float3 sR_b123 = tex2Dlod(fom_shadows_sin, float4(tc.xy, 0, 0)).rgb;

      float fom_distance = tc.z;
      float3 _2pik = PI*float3(1.0,2.0,3.0);
      float3 factor_a = (2.0*fom_distance)*_2pik;

      float3 sin_a123 = sin(factor_a);
      float3 cos_b123 = 1-cos(factor_a);

      float rAtt = (sR_a0123.r*fom_distance/2.0) + dot(sin_a123*(sR_a0123.gba/_2pik), 1) + dot(cos_b123*(sR_b123.rgb/_2pik), 1);
      float2 vignette = saturate( abs(tc.xy*2-1) * 10 - 9 );
      float vignetteEffect = saturate( 1.0 - dot( vignette, vignette ) );
      return lerp(1, saturate(exp(-rAtt)), vignetteEffect);
    }
    ##else
    float getFOMShadow(float3 worldPos){return 1;}
    ##endif
  }
endmacro