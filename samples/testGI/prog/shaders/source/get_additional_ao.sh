include "heightmap_common.sh"
include "giHelpers/heightmap_25d.sh"
define_macro_if_not_defined APPLY_ADDITIONAL_AO(code)
  hlsl(code) {
    float getAdditionalAmbientOcclusion(float ao, float3 worldPos, float3 worldNormal, float2 screenTc)
    { return ao; }
  }
endmacro
