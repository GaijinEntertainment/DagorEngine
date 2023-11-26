texture upscale_sampling_tex;
buffer upscale_sampling_weights;

macro INIT_UPSCALE_SAMPLING_BASE(code)
  (code) {
    upscale_sampling_tex@tex = upscale_sampling_tex
    hlsl {
        Texture2D<float>upscale_sampling_tex@tex;
    }
    upscale_sampling_weights@cbuf = upscale_sampling_weights hlsl {
      #include "upscale_sampling_weights.hlsli"
      cbuffer upscale_sampling_weights@cbuf
      {
        float4 upscale_sampling_weights[UPSCALE_WEIGHTS_COUNT];
      };
    }
  }
endmacro

macro INIT_UPSCALE_SAMPLING()
  INIT_UPSCALE_SAMPLING_BASE(ps)
endmacro

macro USE_UPSCALE_SAMPLING_BASE(code)
  ENABLE_ASSERT(code)
  hlsl(code) {
    float4 SampleUpscaleWeight(int2 screenpos)
    {
      int weightIdx = texelFetch(upscale_sampling_tex, screenpos, 0).x * 255.0;
      return upscale_sampling_weights[weightIdx];
    }
  }
endmacro

macro USE_UPSCALE_SAMPLING()
  USE_UPSCALE_SAMPLING_BASE(ps)
endmacro