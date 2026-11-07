#ifndef TSR_HISTORY_DEPTH_HLSL
#define TSR_HISTORY_DEPTH_HLSL

// tsr_depth holds log2 of linear depth, R16F keeps raw reverse-Z depth too coarse far away
float encode_tsr_depth(float raw_depth, float4 zn_zfar)
{
  return log2(linearize_z(raw_depth, zn_zfar.zw));
}

float decode_tsr_depth(float encoded_depth, float4 zn_zfar)
{
  // R16F rounds the far plane, so it must snap back to the exact sky depth or the sky history is rejected
  const float R16F_RELATIVE_STEP = 1.0 / 1024;
  float encodedFar = log2(zn_zfar.y);
  if (encoded_depth >= encodedFar - abs(encodedFar) * R16F_RELATIVE_STEP)
    return 0;
  return inv_linearizeZ(exp2(encoded_depth), zn_zfar);
}

#endif
