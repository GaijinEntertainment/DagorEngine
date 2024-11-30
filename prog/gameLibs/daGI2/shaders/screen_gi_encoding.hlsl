#ifndef SCREEN_GI_ENCODING
#define SCREEN_GI_ENCODING 1

// we need to encode pixel gi information for temporality
// otherwise it is become "yellowish" due to moving average (each frame we lose some bits)
// there are two good options:
// option 1) store with gamma (say, gamma 2), so loosing some of dynamic range but get back one bit of mantissa
//   even further we can utilize the fact that we store 6 channels (specular and diffuse) simultaneously
//   so we keep diffuse to be all 11 bit (and completely removing any tint!)
//   and specular is r10g10b11, getting 'blueish' tint (rough specular is probably sky)
// option 2) store in rgbe5 format. while it is HW supported on Scarlett, it is not supported (for encoding) everywhere else.
//  so we need to manually encode it. The cost on Xbox One (not Scarlett) is ~5%,
//  but we can reduce a bit by sharing exponent between both colors, making it ~2.5% cost (or 50us) for encoding. decoding is just 10us
// option 3) - use HW RGBE :)

// by default we should use SW RGBE. It is not that expensive and gives best quality
// however, for now, for simplicity of formats compatibility, we keep GI_USE_GAMMA2 (as it is compatible with HW RGBE as well as gamma2)
// adding SW support _by default_ is rather easy, but requires checking of assumption and gi_quality
// i.e for gi_quality == screen_probes & dagi_sp_has_exposure_assume assumed to 1 we have to create texture of uint format
#ifndef GI_PACK_ALGO
#define GI_PACK_ALGO GI_USE_GAMMA2
#endif

#if GI_PACK_ALGO == GI_USE_GAMMA2

#define GI_ENCODED_TYPE half3
#define GI_ENCODED_ATTR xyz
#define GI_ENCODED_CHANNELS 3

void encode_gi_colors(out GI_ENCODED_TYPE color1, out GI_ENCODED_TYPE color2, half3 rgb1, half3 rgb2)
{
  rgb1 = pow2(rgb1);
  rgb2 = pow2(rgb2);
  color1 = half3(rgb1.xy, rgb2.x);
  color2 = half3(rgb1.z, rgb2.zy); // may be divide by ambient?
}

void decode_gi_colors(GI_ENCODED_TYPE color1, GI_ENCODED_TYPE color2, out half3 linear_rgb1, out half3 linear_rgb2)
{
  // safety is not needed, as we have to write everything we read in this pass
  // however, nans are nasty!
  color1 = sqrt(max(0, color1));
  color2 = sqrt(max(0, color2));
  linear_rgb1 = half3(color1.xy, color2.x);
  linear_rgb2 = half3(color1.z, color2.zy);
}

void decode_weighted_colors(half4 r1, half4 g1, half4 b1, half4 r2, half4 g2, half4 b2, out half3 linear1, out half3 linear2, float4 weights)
{
  //safety is needed as values that are read from previous are not guaranteed to be not nans (as we never write pixels with sky)
  linear1.x = dot(sqrt(max(0, select(weights != 0, r1, half4(0,0,0,0)))), weights);
  linear1.y = dot(sqrt(max(0, select(weights != 0, g1, half4(0,0,0,0)))), weights);
  linear1.z = dot(sqrt(max(0, select(weights != 0, r2, half4(0,0,0,0)))), weights);

  linear2.x = dot(sqrt(max(0, select(weights != 0, b1, half4(0,0,0,0)))), weights);
  linear2.y = dot(sqrt(max(0, select(weights != 0, b2, half4(0,0,0,0)))), weights);
  linear2.z = dot(sqrt(max(0, select(weights != 0, g2, half4(0,0,0,0)))), weights);
}

#elif GI_PACK_ALGO == GI_USE_SW_RGBE

#include <pixelPacking/PixelPacking_RGBE.hlsl>

#define GI_ENCODED_TYPE uint
#define GI_ENCODED_CHANNELS 1
#define GI_ENCODED_ATTR x

void encode_gi_colors(out GI_ENCODED_TYPE color1, out GI_ENCODED_TYPE color2, half3 rgb1, half3 rgb2)
{
  // use shared exponent to save some calculations
  // To determine the shared exponent, we must clamp the channels to an expressible range
  const float kMaxVal = asfloat(0x477F8000); // 1.FF x 2^+15
  const float kMinVal = asfloat(0x37800000); // 1.00 x 2^-16

  // Non-negative and <= kMaxVal
  rgb1 = clamp(rgb1, 0, kMaxVal);
  rgb2 = clamp(rgb2, 0, kMaxVal);

  // From the maximum channel we will determine the exponent.  We clamp to a min value
  // so that the exponent is within the valid 5-bit range.
  float MaxChannel = max(max(kMinVal, max(rgb1.r, rgb2.r)), max(max(rgb1.g, rgb2.g), max(rgb1.b, rgb2.b)));

  // 'Bias' has to have the biggest exponent plus 15 (and nothing in the mantissa).  When
  // added to the three channels, it shifts the explicit '1' and the 8 most significant
  // mantissa bits into the low 9 bits.  IEEE rules of float addition will round rather
  // than truncate the discarded bits.  Channels with smaller natural exponents will be
  // shifted further to the right (discarding more bits).
  float Bias = asfloat((asuint(MaxChannel) + 0x07804000) & 0x7F800000);

  // Shift bits into the right places
  uint E = (asuint(Bias) << 4) + 0x10000000;
  uint3 RGB1 = asuint(rgb1 + Bias);
  uint3 RGB2 = asuint(rgb2 + Bias);
  color1 = E | RGB1.b << 18 | RGB1.g << 9 | (RGB1.r & 0x1FF);
  color2 = E | RGB2.b << 18 | RGB2.g << 9 | (RGB2.r & 0x1FF);
}

void decode_gi_colors(GI_ENCODED_TYPE color1, GI_ENCODED_TYPE color2, out half3 linear_rgb1, out half3 linear_rgb2)
{
  linear_rgb1 = UnpackRGBE(color1);
  linear_rgb2 = UnpackRGBE(color2);
}

void decode_weighted_colors(uint4 encoded1, uint4 encoded2, out half3 linear1, out half3 linear2, float4 weights)
{
  half3 a,b;
  decode_gi_colors(encoded1.x, encoded2.x, a, b);
  linear1  = a*weights.x; linear2  = b*weights.x;
  decode_gi_colors(encoded1.y, encoded2.y, a, b);
  linear1 += a*weights.y; linear2 += b*weights.y;
  decode_gi_colors(encoded1.z, encoded2.z, a, b);
  linear1 += a*weights.z; linear2 += b*weights.z;
  decode_gi_colors(encoded1.w, encoded2.w, a, b);
  linear1 += a*weights.w; linear2 += b*weights.w;
}

#elif GI_PACK_ALGO == GI_USE_HW_RGBE

// no encoding (_probably_ hw rgbe5)

#define GI_ENCODED_TYPE half3
#define GI_ENCODED_CHANNELS 3
#define GI_ENCODED_ATTR xyz

void encode_gi_colors(out GI_ENCODED_TYPE color1, out GI_ENCODED_TYPE color2, half3 rgb1, half3 rgb2)
{
  color1 = half3(rgb1.xy, rgb2.x);
  color2 = half3(rgb1.z, rgb2.zy);
  // just in case it is still r11g11b10, swap ambient so it has same bits all over
  //color1 = rgb1;
  //color2 = rgb2;
}

void decode_gi_colors(GI_ENCODED_TYPE color1, GI_ENCODED_TYPE color2, out half3 linear_rgb1, out half3 linear_rgb2)
{
  linear_rgb1 = half3(color1.xy, color2.x);
  linear_rgb2 = half3(color1.z, color2.zy);
}

void decode_weighted_colors(half4 r1, half4 g1, half4 b1, half4 r2, half4 g2, half4 b2, out half3 linear1, out half3 linear2, float4 weights)
{
  // NaNs are nasty, ensure we don't have those
  linear1.x = dot(max(0, r1), weights);
  linear1.y = dot(max(0, g1), weights);
  linear1.z = dot(max(0, r2), weights);
  linear2.x = dot(max(0, b1), weights);
  linear2.y = dot(max(0, b2), weights);
  linear2.z = dot(max(0, g2), weights);
}

#elif GI_PACK_ALGO == GI_USE_NO_ENCODING

// no encoding

#define GI_ENCODED_TYPE half3
#define GI_ENCODED_CHANNELS 3
#define GI_ENCODED_ATTR xyz

void encode_gi_colors(out GI_ENCODED_TYPE color1, out GI_ENCODED_TYPE color2, half3 rgb1, half3 rgb2)
{
  color1 = rgb1;
  color2 = rgb2;
}

void decode_gi_colors(GI_ENCODED_TYPE color1, GI_ENCODED_TYPE color2, out half3 linear_rgb1, out half3 linear_rgb2)
{
  linear_rgb1 = color1;
  linear_rgb2 = color2;
}

void decode_weighted_colors(half4 r1, half4 g1, half4 b1, half4 r2, half4 g2, half4 b2, out half3 linear1, out half3 linear2, float4 weights)
{
  linear1.x = dot(max(0, r1), weights);
  linear1.y = dot(max(0, g1), weights);
  linear1.z = dot(max(0, b1), weights);
  linear2.x = dot(max(0, r2), weights);
  linear2.y = dot(max(0, g2), weights);
  linear2.z = dot(max(0, b2), weights);
}

#endif

#endif