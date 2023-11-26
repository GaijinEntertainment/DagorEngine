macro ATMO(code)
  hlsl(code) {
    #ifndef ATMOSPHERE_INITED
    #define ATMOSPHERE_INITED 1
    #include "atmosphere/definitions.hlsli"
    #include "atmosphere/atmosphere_params.hlsli"
    #include "atmosphere/definitions_units.hlsli"
    #include "atmosphere/texture_sizes.hlsli"
    #include "atmosphere/transmittance.hlsli"
    #include "atmosphere/functions.hlsli"
    //float3 luminance_from_radiance(float3 r){return r;}
    #endif
  }
endmacro

buffer atmosphere_cb;

macro GET_ATMO(code)
  (code) {
    atmosphere_cb@cbuf = atmosphere_cb hlsl {
      #include "atmosphere/definitions.hlsli"
      #include "atmosphere/atmosphere_params.hlsli"
      cbuffer atmosphere_cb@cbuf
      {
        AtmosphereParameters theAtmosphere;
      };
    }
  }
endmacro

