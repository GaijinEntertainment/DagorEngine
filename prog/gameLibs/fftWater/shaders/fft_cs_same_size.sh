//this is optimized for same size (in one dispatch)
// techically it still workds with different sizes dispatches, but will be sub-optimal

include "../../render/shaders/hardware_defines.sh"
include "fft_cs_defaults.sh"
include "fft_cs_compression.sh"

int fft_spectrum = 0;
interval fft_spectrum : phillips < 1, unified_directional;

macro CBUFFER()
  hlsl(cs) {
    cbuffer MyConstantBuffer : register(b0) 
    {
      struct CB
      {
        #include <fft_water_cbuffer.hlsli>
      } cb_array[4];
    }
  }
endmacro

shader update_h0_cs, update_h0_cs_android
{
  if (shader == update_h0_cs_android) {
    DISABLE_FFT_COMPRESSION()
  }

  ENABLE_ASSERT(cs)
  USE_FFT_COMPRESSION()
  CBUFFER()
  hlsl(cs) {
    #include "fft_spectrum.hlsli"
##if fft_spectrum == phillips
    #define calcSpectrum calcPhillips
##elif fft_spectrum == unified_directional
    #define calcSpectrum calcUnifiedDirectional
##endif

    StructuredBuffer<float2> g_gauss_input : register(t0);
    RWStructuredBuffer<h0_type> g_h0_output : register(u0);

    // update H0 from Gauss (one CTA per row)
    [numthreads(CS_MAX_FFT_RESOLUTION, 1, 1)]
    void ComputeH0( uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID )
    {
      CB cb = cb_array[groupId.z];
      uint h0_index0 = groupId.z* cb.m_resolution_plus_one_squared_minus_one + groupId.z;
      uint columnIdx = dispatchThreadId.x;
      uint rowIdx = dispatchThreadId.y;

      if (columnIdx < cb.m_resolution) 
      {
        int nx = columnIdx - cb.m_half_resolution;
        int ny = rowIdx - cb.m_half_resolution;
        float nr = sqrt(float(nx*nx + ny*ny));

        float amplitude = 0.0f;
        if ((nx || ny) && nr >= cb.m_window_in && nr < cb.m_window_out)
        {
          float2 k = float2(nx * cb.m_frequency_scale, ny * cb.m_frequency_scale);
          amplitude = cb.m_linear_scale * sqrt(calcSpectrum(k, cb.m_wind_dir, cb.m_root_scale, 1, cb.m_wind_scale, cb.m_power_scale, cb.m_wind_align));
        }

        int index = rowIdx * cb.m_resolution_plus_one + columnIdx;
        float2 h0 = amplitude * structuredBufferAt(g_gauss_input, index - rowIdx);
        structuredBufferAt(g_h0_output, h0_index0+index) = encode_float2_to_h0type(h0);

        // mirror first row/column, CPU and CUDA paths don't do that
        // however, we need to initialize the N+1'th row/column to zero 
        if (!rowIdx || !columnIdx)
          structuredBufferAt(g_h0_output, h0_index0+cb.m_resolution_plus_one_squared_minus_one - index) = 0; //h0;
      }
    }
  }
  compile("cs_5_0", "ComputeH0");
}

shader fftH_cs, fftV_cs, fftH_cs_android, fftV_cs_android
{
  if (shader == fftH_cs_android || shader == fftV_cs_android) {
    DISABLE_FFT_COMPRESSION()
  }
  ENABLE_ASSERT(cs)
  USE_FFT_COMPRESSION()
  CBUFFER()
  hlsl(cs) {
    groupshared float2 uData[CS_MAX_FFT_RESOLUTION/2];
    groupshared float2 vData[CS_MAX_FFT_RESOLUTION/2];
    groupshared float2 wData[CS_MAX_FFT_RESOLUTION/2];

    uint reverse_bits32( uint bits )
    {
    ##if !hardware.metaliOS
      return reversebits( bits );
    ##else
      bits = ( bits << 16) | ( bits >> 16);
      bits = ( (bits & 0x00ff00ff) << 8 ) | ( (bits & 0xff00ff00) >> 8 );
      bits = ( (bits & 0x0f0f0f0f) << 4 ) | ( (bits & 0xf0f0f0f0) >> 4 );
      bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
      bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
      return bits;
    ##endif
    }

    // input is bit-reversed threadIdx and threadIdx+1
    // output is threadIdx and threadIdx + resolution/2
    void fft(inout float2 u[2], inout float2 v[2], inout float2 w[2], uint threadIdx, uint half_resolution, uint resolution)
    {
      bool flag = false;
      float scale = 3.14159265359f * 0.5f; // Pi

      if(threadIdx < half_resolution)
      {
        {
          uint i = threadIdx;

          float2 du = u[1]; 
          float2 dv = v[1]; 
          float2 dw = w[1]; 
            
          u[1] = u[0] - du; 
          u[0] = u[0] + du;
          v[1] = v[0] - dv; 
          v[0] = v[0] + dv;
          w[1] = w[0] - dw; 
          w[0] = w[0] + dw;
          
          flag = threadIdx & 1;

          // much slower: vData[i] = v[!flag];
          if(flag) 
          {
            uData[i] = u[0]; 
            vData[i] = v[0]; 
            wData[i] = w[0]; 
          } else {
            uData[i] = u[1]; 
            vData[i] = v[1]; 
            wData[i] = w[1]; 
          }
        }
      }
      GroupMemoryBarrierWithGroupSync();

      uint stride;
    ##if !hardware.ps4 && !hardware.ps5
      [unroll(WARP_WIDTH_SHIFT-1)] // log2(WARP_WIDTH) - 1
    ##endif
      for(stride = 2; stride < WARP_WIDTH; stride <<= 1, scale *= 0.5f)
      {
        if(threadIdx < half_resolution)
        {
          uint i = threadIdx ^ (stride-1);
          uint j = threadIdx & (stride-1);

          // much slower: v[!flag] = vData[i];
          if(flag) 
          {
            u[0] = uData[i]; 
            v[0] = vData[i]; 
            w[0] = wData[i]; 
          } else { 
            u[1] = uData[i];
            v[1] = vData[i];
            w[1] = wData[i];
          }

          float sin, cos;
          sincos(j * scale, sin, cos);

          float2 du = float2(
            cos * u[1].x - sin * u[1].y, 
            sin * u[1].x + cos * u[1].y);
          float2 dv = float2(
            cos * v[1].x - sin * v[1].y, 
            sin * v[1].x + cos * v[1].y);
          float2 dw = float2(
            cos * w[1].x - sin * w[1].y, 
            sin * w[1].x + cos * w[1].y);

          u[1] = u[0] - du;
          u[0] = u[0] + du;
          v[1] = v[0] - dv;
          v[0] = v[0] + dv;
          w[1] = w[0] - dw;
          w[0] = w[0] + dw;

          flag = threadIdx & stride;

          // much slower: vData[i] = v[!flag];
          if(flag) 
          {
            uData[i] = u[0]; 
            vData[i] = v[0]; 
            wData[i] = w[0]; 
          } else { 
            uData[i] = u[1];
            vData[i] = v[1];
            wData[i] = w[1];
          }
        }
        GroupMemoryBarrierWithGroupSync();
      }

    ##if !hardware.ps4 && !hardware.ps5
      [unroll(MAX_FFT_RESOLUTION_SHIFT-WARP_WIDTH_SHIFT)] // log2(MAX_FFT_RESOLUTION) - log2(WARP_WIDTH)
    ##endif
      for(stride = WARP_WIDTH; stride < resolution; stride <<= 1, scale *= 0.5f)
      {
        if (threadIdx < half_resolution)
        {
          uint i = threadIdx ^ (stride-1);
          uint j = threadIdx & (stride-1);

          // much slower: v[!flag] = vData[i];
          if(flag) 
          {
            u[0] = uData[i]; 
            v[0] = vData[i]; 
            w[0] = wData[i]; 
          } else { 
            u[1] = uData[i];
            v[1] = vData[i];
            w[1] = wData[i];
          }

          float sin, cos;
          sincos(j * scale, sin, cos);

          float2 du = float2(
            cos * u[1].x - sin * u[1].y, 
            sin * u[1].x + cos * u[1].y);
          float2 dv = float2(
            cos * v[1].x - sin * v[1].y, 
            sin * v[1].x + cos * v[1].y);
          float2 dw = float2(
            cos * w[1].x - sin * w[1].y, 
            sin * w[1].x + cos * w[1].y);

          u[1] = u[0] - du;
          u[0] = u[0] + du;
          v[1] = v[0] - dv;
          v[0] = v[0] + dv;
          w[1] = w[0] - dw;
          w[0] = w[0] + dw;

          flag = threadIdx & stride;

          // much slower: vData[i] = v[!flag];
          if(flag) 
          {
            uData[i] = u[0]; 
            vData[i] = v[0]; 
            wData[i] = w[0]; 
          } else { 
            uData[i] = u[1];
            vData[i] = v[1];
            wData[i] = w[1];
          }
        }
        
        GroupMemoryBarrierWithGroupSync();
      }
    }

    StructuredBuffer<h0_type> g_h0_input : register(t0);
    StructuredBuffer<float> g_omega_input : register(t1);

    RWStructuredBuffer<ht_type> g_ht_output : register(u0);
    RWStructuredBuffer<dt_type> g_dt_output : register(u1);

    // update Ht, Dt_x, Dt_y from H0 and Omega, fourier transform per row (one CTA per row)
    [numthreads(CS_MAX_FFT_RESOLUTION/2, 1, 1)]
    void ComputeRows( uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
    {
      CB cb = cb_array[groupId.z];
      uint columnIdx = dispatchThreadId.x * 2;
      uint rowIdx = dispatchThreadId.y;
      uint reverseColumnIdx = reverse_bits32(columnIdx) >> cb.m_32_minus_log2_resolution;
      int3 n = int3(reverseColumnIdx - cb.m_half_resolution, reverseColumnIdx, rowIdx - cb.m_half_resolution);

      float2 ht[2], dx[2], dy[2];
      if(columnIdx < cb.m_resolution) 
      {
        float4 h0i, h0j;
        float2 omega;

        uint h0_index0 = groupId.z* cb.m_resolution_plus_one_squared_minus_one + groupId.z;
        uint omega_index0 = groupId.z* cb.m_half_resolution_plus_one* cb.m_half_resolution_plus_one;

        uint h0_index = ((rowIdx<<cb.m_resolution_bits) + rowIdx)  + reverseColumnIdx;
        uint h0_jndex = h0_index + cb.m_half_resolution;
        uint omega_index = ((rowIdx<<cb.m_resolution_bits_1) + rowIdx);
        uint omega_jndex = omega_index + cb.m_half_resolution;

        h0i.xy = decode_float2_h0type(structuredBufferAt(g_h0_input, h0_index0+h0_index));
        h0j.xy = decode_float2_h0type(structuredBufferAt(g_h0_input, h0_index0+cb.m_resolution_plus_one_squared_minus_one - h0_index));
        omega.x = structuredBufferAt(g_omega_input, omega_index0+omega_index + reverseColumnIdx) * cb.m_time;

        h0i.zw = decode_float2_h0type(structuredBufferAt(g_h0_input, h0_index0+h0_jndex));
        h0j.zw = decode_float2_h0type(structuredBufferAt(g_h0_input, h0_index0+cb.m_resolution_plus_one_squared_minus_one - h0_jndex));
        omega.y = structuredBufferAt(g_omega_input, omega_index0+omega_jndex - reverseColumnIdx) * cb.m_time;

        // modulo 2 * Pi
        const float oneOverTwoPi = 0.15915494309189533576888376337251;
        const float twoPi = 6.283185307179586476925286766559;
        omega -= floor(float2(omega * oneOverTwoPi)) * twoPi;

        float2 sinOmega, cosOmega;
        sincos(float2(omega), sinOmega, cosOmega);

        // H(0) -> H(t)
        ht[0].x = (h0i.x + h0j.x) * cosOmega.x - (h0i.y + h0j.y) * sinOmega.x;
        ht[1].x = (h0i.z + h0j.z) * cosOmega.y - (h0i.w + h0j.w) * sinOmega.y;
        ht[0].y = (h0i.x - h0j.x) * sinOmega.x + (h0i.y - h0j.y) * cosOmega.x;
        ht[1].y = (h0i.z - h0j.z) * sinOmega.y + (h0i.w - h0j.w) * cosOmega.y;

        float2 nr = n.xy || n.z ? rsqrt(float2(n.xy*n.xy + n.z*n.z)) : 0;
        float2 dt0 = float2(-ht[0].y, ht[0].x) * nr.x;
        float2 dt1 = float2(-ht[1].y, ht[1].x) * nr.y;

        dx[0] = n.x * dt0;
        dx[1] = n.y * dt1;
        dy[0] = n.z * dt0;
        dy[1] = n.z * dt1;
      }

      fft(ht, dx, dy, dispatchThreadId.x, cb.m_half_resolution, cb.m_resolution);

      if(columnIdx < cb.m_resolution)
      {
        uint index0 = (groupId.z + (groupId.z<<cb.m_resolution_bits_1))<<cb.m_resolution_bits;
        uint index = index0 + (rowIdx << cb.m_resolution_bits) + dispatchThreadId.x;
        g_ht_output[index] = encode_float2_to_ht_type(ht[0]);
        g_ht_output[index+cb.m_half_resolution] = encode_float2_to_ht_type(ht[1]);

        structuredBufferAt(g_dt_output, index) = encode_float4_to_dt_type(float4(dx[0], dy[0]));
        structuredBufferAt(g_dt_output, index+cb.m_half_resolution) = encode_float4_to_dt_type(float4(dx[1], dy[1]));
      }
    }

    StructuredBuffer<ht_type> g_ht_input : register(t0);
    StructuredBuffer<dt_type> g_dt_input : register(t1);

    RWTexture2DArray<float4> g_displacement_output : register(u0);

    // do fourier transform per row of Ht, Dt_x, Dt_y, write displacement texture (one CTA per column)
    [numthreads(CS_MAX_FFT_RESOLUTION/2, 1, 1)]
    void ComputeColumns( uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID)
    {
      CB cb = cb_array[groupId.z];
      uint rowIdx = dispatchThreadId.x * 2;
      uint columnIdx = dispatchThreadId.y;
      uint reverseRowIdx = reverse_bits32(rowIdx) >> cb.m_32_minus_log2_resolution;
      uint index0 = (groupId.z + (groupId.z<<cb.m_resolution_bits_1))<<cb.m_resolution_bits;

      uint index = index0 + (reverseRowIdx << cb.m_resolution_bits) + columnIdx;
      int jndex = index0 + ((cb.m_half_resolution - reverseRowIdx) << cb.m_resolution_bits) + columnIdx;

      float2 ht[2], dx[2], dy[2];
      if(rowIdx < cb.m_resolution)
      {
        ht[0] = decode_float2_from_ht_type(structuredBufferAt(g_ht_input, index));
        ht[1] = decode_float2_from_ht_type(structuredBufferAt(g_ht_input, jndex));
        ht[1].y = -ht[1].y;

        float4 dti = decode_float4_from_dt_type(g_dt_input[index]);
        float4 dtj = decode_float4_from_dt_type(g_dt_input[jndex]);

        dx[0] = dti.xy;
        dx[1] = float2(dtj.x, -dtj.y);
        dy[0] = dti.zw;
        dy[1] = float2(dtj.z, -dtj.w);
      }

      fft(ht, dx, dy, dispatchThreadId.x, cb.m_half_resolution, cb.m_resolution);

      if(rowIdx < cb.m_resolution)
      {
        float sgn = (dispatchThreadId.x + columnIdx) & 0x1 ? -1.0f : +1.0f;
        float scale = cb.m_choppy_scale * sgn;
        uint arrayOutId = groupId.z;
        g_displacement_output[uint3(dispatchThreadId.x, columnIdx, arrayOutId)] =
          float4(dx[0].x * scale, dy[0].x * scale, ht[0].x * sgn, 0);
        g_displacement_output[uint3(dispatchThreadId.x + cb.m_half_resolution, columnIdx, arrayOutId)] =
          float4(dx[1].x * scale, dy[1].x * scale, ht[1].x * sgn, 0);
      }
    }
  }
  if (shader == fftH_cs || shader == fftH_cs_android)
  {
    compile("cs_5_0", "ComputeRows");
  } else
  {
    hlsl {
      #define __XBOX_REGALLOC_VGPR_LIMIT 32//found by pix
    }
    compile("cs_5_0", "ComputeColumns");
  }
}
