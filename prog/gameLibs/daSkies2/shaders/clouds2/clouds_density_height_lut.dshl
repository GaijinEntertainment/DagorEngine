macro DENSITY_HEIGHT_GRADIENT_LUT(code)
  hlsl(code) {
    #include <clouds_density_height_lut.hlsli>
    float2 encodeCloudsTypeTC(float cloud_type, float heightFraction)
    {
      return float2(heightFraction, cloud_type*((CLOUDS_TYPES_LUT-1.)/CLOUDS_TYPES_LUT)+ 0.5/CLOUDS_TYPES_LUT);
    }
    void decodeCloudsTypeTC(float2 tc, out float cloud_type, out float heightFraction)
    {
      //intentionally ignore correct normalization. we use border addressing, and we know heightFraction = 0,1 gives us zero anyway.
      //so, we both save math/alu AND increase texture utilization
      heightFraction  = tc.x;//*CLOUDS_TYPES_HEIGHT_LUT/(CLOUDS_TYPES_HEIGHT_LUT-1.) - 0.5/(CLOUDS_TYPES_HEIGHT_LUT-1.);
      cloud_type      = tc.y*CLOUDS_TYPES_LUT/(CLOUDS_TYPES_LUT-1.) - 0.5/(CLOUDS_TYPES_LUT-1.);
    }
  }
endmacro

texture clouds_types_lut;

macro DENSITY_HEIGHT_GRADIENT_TEXTURE(code)
  DENSITY_HEIGHT_GRADIENT_LUT(code)
  (code) { clouds_types_lut@smp2d = clouds_types_lut;}
  hlsl(code) {
    void getCloudsTypeParamsTex(float cloud_type, float heightFraction, float cloud_coverage,
       out float density_height_gradient1, out float density_height_gradient2, out float erosionLevel)
    {
      float4 v = tex2Dlod(clouds_types_lut, float4(encodeCloudsTypeTC(cloud_type,heightFraction), 0,0));
      density_height_gradient1 = v.x;
      density_height_gradient2 = v.y;
      erosionLevel = (v.z - v.w*cloud_coverage);
    }
  }
endmacro
