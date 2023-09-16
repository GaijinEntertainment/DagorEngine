
buffer spot_lights_flags;
buffer omni_lights_flags;

hlsl {
  #define check_byte_buffer_by_mask(flag_buffer, index, mask) (flag_buffer[index/4] & (mask << ((index&3)<<3)))
}

macro USE_SPOT_LIGHT_MASK(code)
  (code) {spot_lights_flags@buf = spot_lights_flags hlsl {StructuredBuffer<uint> spot_lights_flags@buf;}; }
  hlsl(code) {
    #define CHECK_SPOT_LIGHT_MASK 1U

    bool check_spot_light(uint light_index){ return check_byte_buffer_by_mask(spot_lights_flags, light_index, CHECK_SPOT_LIGHT_MASK); }
  }
endmacro

macro USE_OMNI_LIGHT_MASK(code)
  (code) {omni_lights_flags@buf = omni_lights_flags hlsl {StructuredBuffer<uint> omni_lights_flags@buf;}; }
  hlsl(code) {
    #define CHECK_OMNI_LIGHT_MASK 1U

    bool check_omni_light(uint light_index){ return check_byte_buffer_by_mask(omni_lights_flags, light_index, CHECK_OMNI_LIGHT_MASK); }
  }
endmacro
