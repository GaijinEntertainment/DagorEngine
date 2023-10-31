// If the FSR was used during gbuffer rendering, then this var will have YES for the whole frame
int fsr_distortion = 0;
interval fsr_distortion: no < 1, yes;

int fsr_lut_id = 0;

macro USE_FSR(code)
  if (hardware.ps5 && fsr_distortion == yes)
  {
    (code)
    {
      fsr_lut_id@i1 = fsr_lut_id;
      fsr_viewport@f4 = get_viewport();
    }

    hlsl(code)
    {
      RegularBuffer<sce::Gnm::Texture> static_textures : BINDLESS_TEX_REGISTER;
      RegularBuffer<sce::Gnm::Sampler> static_samplers : BINDLESS_SAMPLER_REGISTER;

      uint2 fsrPos(uint2 pos, uint2 size)
      {
        Texture1D<float> fsrLutH = static_textures[fsr_lut_id];
        Texture1D<float> fsrLutV = static_textures[fsr_lut_id + 1];
        SamplerState smp = static_samplers[0];

        float2 tc = float2(pos) / float2(size);
        return uint2(fsrLutH.Sample(smp, tc.x) * size.x, fsrLutV.Sample(smp, tc.y) * size.y);
      }

      float2 getTexcoord(float2 pos)
      {
        return (pos - fsr_viewport.xy) / fsr_viewport.zw;
      }

      float2 getTexcoord(float2 pos, float2 texcoord)
      {
        return getTexcoord(pos);
      }

      #define FSR_DISTORTION 1
    }
  }
  else
  {
    hlsl(code)
    {
      uint2 fsrPos(uint2 pos, uint2 size)
      {
        return pos;
      }

      float2 getTexcoord(float2 pos, float2 texcoord)
      {
        return texcoord;
      }
    }
  }
endmacro
