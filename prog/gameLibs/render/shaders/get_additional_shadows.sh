
define_macro_if_not_defined APPLY_ADDITIONAL_SHADOWS(code)
  hlsl(code) {
    float getAdditionalShadow(float shadow, float3 cameraToOrigin, float3 worldNormal){return shadow;}
  }
endmacro
