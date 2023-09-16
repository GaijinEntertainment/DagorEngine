macro WRITE_GBUFFER()
hlsl(ps) {
  struct GBUFFER_OUTPUT { half4 dummy : SV_Target0; };
}
endmacro

macro PACK_UNPACK_GBUFFER()
hlsl(ps) {
  struct UnpackedGbuffer { float dummy; };

  void init_gbuffer(out UnpackedGbuffer result) { result = (UnpackedGbuffer)0; }
  void init_albedo(inout UnpackedGbuffer result, half3 albedo) {}
  void init_smoothness(inout UnpackedGbuffer result, half smoothness) {}
  void init_normal(inout UnpackedGbuffer result, float3 norm) {}
  void init_metalness(inout UnpackedGbuffer result, half metal) {}
  void init_reflectance(inout UnpackedGbuffer result, float reflectance) {}
  void init_dynamic(inout UnpackedGbuffer result, bool dynamic) {}
  void init_motion_vector(inout UnpackedGbuffer result, half2 motion) {}

  GBUFFER_OUTPUT encode_gbuffer(UnpackedGbuffer gbuf, float3 pointToEye) { return (GBUFFER_OUTPUT)0; }
  #define encode_gbuffer3(a,b,c) encode_gbuffer(a, b)  // For compatibility of shared shaders with WT which has 3 parameters.
}
endmacro
