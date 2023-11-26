macro DISABLE_FFT_COMPRESSION()
  hlsl(cs) {
    #define H0_COMPRESSION 0
    #define FFT_COMPRESSION 0
  }
endmacro

macro USE_FFT_COMPRESSION()
  hlsl(cs) {
    #include <fft_water_defs.hlsli>
    #if FFT_COMPRESSION
      float2 decode_float2_from_ht_type(ht_type val)
      {
        return float2(f16tof32(val), f16tof32(val>>16));
      }
      float4 decode_float4_from_dt_type(dt_type val)
      {
        return float4(f16tof32(val), f16tof32(val>>16));
      }
      ht_type encode_float2_to_ht_type(float2 val)
      {
        uint2 ival = f32tof16(val);
        return ival.x|(ival.y<<16);
      }
      dt_type encode_float4_to_dt_type(float4 val)
      {
        uint4 ival = f32tof16(val);
        return uint2(ival.x|(ival.z<<16), ival.y|(ival.w<<16));
      }
    #else
      float2 decode_float2_from_ht_type(float2 val) {return val;}
      float4 decode_float4_from_dt_type(float4 val) {return val;}
      float2 encode_float2_to_ht_type(float2 val) {return val;}
      float4 encode_float4_to_dt_type(float4 val) {return val;}
    #endif

    #if H0_COMPRESSION
      float2 decode_float2_h0type(uint val)
      {
        return float2(f16tof32(val), f16tof32(val>>16));
      }
      uint encode_float2_to_h0type(float2 val)
      {
        uint2 ival = f32tof16(val);
        return ival.x|(ival.y<<16);
      }
    #else
      float2 decode_float2_h0type(float2 val) {return val;}
      float2 encode_float2_to_h0type(float2 val) {return val;}
    #endif
  }
endmacro
