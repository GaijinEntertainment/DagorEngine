include "shader_global.sh"

// this is an implementation of AMD's CAS with some minor tweaks

macro USE_CONTRAST_ADAPTIVE_SHARPENING(code)
  hlsl(code) {
    #include <pixelPacking/yCoCgSpace.hlsl>

    half4 tonemap(half4 color)
    {
      return half4(color.rgb * rcp(abs(color.r) + 1), color.a);
    }

    half4 tonemap_inv(half4 color)
    {
      return half4(color.rgb * rcp(max(1e-5, 1 - abs(color.r))), color.a);
    }

    half4 tex2DCAS(Texture2D tex, int2 pixelCoord, float sharpening)
    {
      half4 a = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(-1, -1), 0))));
      half4 b = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(0, -1), 0))));
      half4 c = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(1, -1), 0))));
      half4 d = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(-1, 0), 0))));
      half4 e = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord, 0))));
      half4 f = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(1, 0), 0))));
      half4 g = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(-1, 1), 0))));
      half4 h = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(0, 1), 0))));
      half4 i = tonemap(PackToYCoCgAlpha(tex.Load(int3(pixelCoord + int2(1, 1), 0))));

      // Soft min and max.
      //  a b c             b
      //  d e f * 0.5  +  d e f * 0.5
      //  g h i             h
      // These are 2.0x bigger (factored out the extra multiply).
      half mn = min3(min3(d.x, e.x, f.x), b.x, h.x);
      half mn2 = min3(min3(mn, a.x, c.x), g.x, i.x);
      mn+=mn2;
      half mx = max3(max3(d.x, e.x, f.x), b.x, h.x);
      half mx2 = max3(max3(mx, a.x, c.x), g.x, i.x);
      mx+=mx2;

      // Smooth minimum distance to signal limit divided by smooth max.
      half amp = saturate(min(mn, 2.0 - mx) * rcp(mx));

      // Shaping amount of sharpening.
      amp = sqrt(amp);
      // Filter shape.
      //  0 w 0
      //  w 1 w
      //  0 w 0
      half w = amp * sharpening;

      return UnpackFromYCoCgAlpha(tonemap_inv((b*w + d*w + f*w + h*w + e) * rcp(1.0 + 4.0 * w)));
    }
  }
endmacro