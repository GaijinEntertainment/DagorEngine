macro INIT_MOTION_VEC_ENCODE(stage)
endmacro

macro INIT_MOTION_VEC_DECODE(stage)
endmacro

macro USE_MOTION_VEC_ENCODE(stage)
  hlsl(stage) {
    float2 encode_motion_vector(float2 motion_in_uv)
    {
      return motion_in_uv * 256;
    }
    float2 encode_motion_vector(float4 out_pos, float4 out_pos_prev)
    {
      float2 sstc0 = out_pos.xy / out_pos.w;
      float2 sstc1 = out_pos_prev.xy / out_pos_prev.w;
      float2 motion = sstc1.xy - sstc0.xy;
      return encode_motion_vector(motion * float2(0.5, -0.5));
    }
  }
endmacro

macro USE_MOTION_VEC_DECODE(stage)
  hlsl(stage) {
    float2 decode_motion_vector(float2 motion)
    {
      return motion * (1.0 / 256);
    }
  }
endmacro
