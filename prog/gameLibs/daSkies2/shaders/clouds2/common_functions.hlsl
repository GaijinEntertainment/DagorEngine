  float set_range(float value, float low, float high) {
    return saturate((value - low)/(high - low));
  }
  float3 set_ranges_signed(float3 values, float low, float high) {
    return (values - low)/(high - low);
  }

  float set_range_clamped(float value, float low, float high) {
    float ranged_value = clamp(value, low, high);
    ranged_value = (ranged_value - low)/(high - low);
    return saturate(ranged_value);
  }

  half2 encode_curl(half3 c) {
    return c.xy;//*0.5+0.5; //use R8G8S
  }

  half2 decode_curl(half2 c) {
    return c;//*2-1; //use R8G8S
  }
