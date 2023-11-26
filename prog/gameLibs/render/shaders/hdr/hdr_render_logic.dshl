macro HDR_LOGIC(code)
  (code) {
    hdr_params@f4 = (paper_white_nits / 100, paper_white_nits / 10000, max(-0.5, 1 - hdr_shadows), 1 / (hdr_brightness + 0.01));
  }

  hlsl(code) {
    #include "pixelPacking/ColorSpaceUtility.hlsl"

    static const float3x3 from709to2020 =
    {
      { 0.6274040f, 0.3292820f, 0.0433136f },
      { 0.0690970f, 0.9195400f, 0.0113612f },
      { 0.0163916f, 0.0880132f, 0.8955950f }
    };

    float3 to_linear(float3 srgbColor)
    {
      float3 linearColor = accurateSRGBToLinear(srgbColor);
      linearColor = pow(linearColor, hdr_params.w);

      const float shadows = 0.2;
      float lum = luminance(linearColor);
      linearColor *= lum < shadows ? pow(lum / shadows, hdr_params.z) : 1;
      return linearColor;
    }

    float3 encode_hdr_value(float3 linearColor, bool hdr10)
    {
      if (hdr10)
      {
        float3 rec2020 = mul(from709to2020, linearColor);
        float3 normalizedLinearValue = rec2020 * hdr_params.y; // Normalize to max HDR nits.
        float3 hdrRresult = LinearToREC2084(normalizedLinearValue);
        return hdrRresult;
      }
      else
        return linearColor * hdr_params.x;
    }
  }
endmacro