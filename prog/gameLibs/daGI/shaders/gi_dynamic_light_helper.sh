define_macro_if_not_defined GI_DYNAMIC_LIGHT_HELPER(code)
  hlsl(code) {
      half3 getDynamicLighting(half3 normal, half3 albedo, float3 worldPos)
      {
        return (half3)0;
      }
  }
endmacro