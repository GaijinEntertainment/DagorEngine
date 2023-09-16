macro USE_GAUSSIAN_BLUR_COMMON()
  hlsl(ps) {
    GAUSSIAN_BLUR_COLOR_TYPE GaussianBlur(float2 centreUV, float2 pixelOffset
    #if GAUSSIAN_BLUR_USE_BLUR_BOX
      , float4 box)
    #else
      )
    #endif
    {
    #if GAUSSIAN_BLUR_STEPS_COUNT == 2
      float gWeights[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.450429, 0.0495707 };
      float gOffsets[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.53608, 2.06049 };
    #elif GAUSSIAN_BLUR_STEPS_COUNT == 3
      float gWeights[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.329654, 0.157414, 0.0129324 };
      float gOffsets[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.622052, 2.274, 4.14755 };
    #elif GAUSSIAN_BLUR_STEPS_COUNT == 4
      float gWeights[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.24956, 0.192472, 0.0515112, 0.00645659 };
      float gOffsets[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.644353, 2.37891, 4.2912, 6.21672 };
    #elif GAUSSIAN_BLUR_STEPS_COUNT == 6
      float gWeights[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.16501, 0.17507, 0.10112, 0.04268, 0.01316, 0.00296 };
      float gOffsets[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.65772, 2.45017, 4.41096, 6.37285, 8.33626, 10.30153 };
    #elif GAUSSIAN_BLUR_STEPS_COUNT == 9
      float gWeights[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.10855, 0.13135, 0.10406, 0.07216, 0.04380, 0.02328, 0.01083, 0.00441, 0.00157 };
      float gOffsets[GAUSSIAN_BLUR_STEPS_COUNT] = { 0.66293, 2.47904, 4.46232, 6.44568, 8.42917, 10.41281, 12.39664, 14.38070, 16.36501 };
    #else
      #error Unsupported number of gaussian blur steps GAUSSIAN_BLUR_STEPS_COUNT
    #endif

      GAUSSIAN_BLUR_COLOR_TYPE colOut = 0;

      UNROLL
      for(int i = 0; i < GAUSSIAN_BLUR_STEPS_COUNT; i++)
      {
        float2 texCoordOffset = gOffsets[i] * pixelOffset;
        GAUSSIAN_BLUR_COLOR_TYPE col;
      #if GAUSSIAN_BLUR_USE_BLUR_BOX
        gaussianBlurSampleColor(centreUV, texCoordOffset, box, col);
      #else
        gaussianBlurSampleColor(centreUV, texCoordOffset, col);
      #endif
        colOut += gWeights[i] * col;
      }

      return colOut;
    }
  }
endmacro

macro USE_GAUSSIAN_BLUR_BOX()
  hlsl(ps) {
    #define GAUSSIAN_BLUR_USE_BLUR_BOX 1
  }
  USE_GAUSSIAN_BLUR_COMMON()
endmacro

macro USE_GAUSSIAN_BLUR()
  USE_GAUSSIAN_BLUR_COMMON()
endmacro