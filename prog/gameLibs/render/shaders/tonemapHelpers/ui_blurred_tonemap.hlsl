#ifndef UI_BLURRED_TONEMAP
#define UI_BLURRED_TONEMAP 1
#define ADDITIONAL_TONEMAP 1

half3 additional_ui_blurred_tonemap(half3 ret)
{
  #if ADDITIONAL_TONEMAP
    half startAttenuateAt = 0.7;
    half attenuateAtOne = 0.92;

    half lum = luminance(ret);
    half mulRet = 1;
    FLATTEN
    if (lum > startAttenuateAt && lum < 1.2)
    {
      //f(x) = ax+b;
      //f(startAttenuateAt): a*startAttenuateAt + b = 1
      //f(1): a + b = attenuateAtOne
      //a = (attenuateAtOne-1)/(1-startAttenuateAt)
      //b = attenuateAtOne-a
      half attenuateMul = (attenuateAtOne-1)/(1-startAttenuateAt);
      half attenuateOfs = attenuateAtOne - attenuateMul;
      mulRet = saturate(attenuateOfs + attenuateMul*lum);
    }
    ret = ret*mulRet;
  #endif
  return saturate(ret);
}
#endif