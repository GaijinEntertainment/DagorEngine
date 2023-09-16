define_macro_if_not_defined GET_ALTERNATE_REFLECTIONS(code)
  hlsl {
    void get_alternate_reflections(inout half4 newTarget, uint2 pixelPos, float3 cameraToPoint, float3 normal, bool hit, bool hit_not_zfar){}
  }
endmacro