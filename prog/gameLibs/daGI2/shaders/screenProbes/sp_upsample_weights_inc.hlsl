#include <screenprobes_consts.hlsli>
#include <sp_def_precision.hlsl>

uint4 load_encoded_screen_probes(UseBufferInfo srvInfo, uint2 probeCoord2D, int screenspace_probe_stride)
{
  uint4 encodedProbes;
  UNROLL
  for (uint i = 0; i < 4; ++i)
  {
    uint2 coordOffset = uint2(i&1, (i&2)>>1);
    uint2 sampleProbeCoord = coordOffset + probeCoord2D;
    uint2 atlasSampleProbeCoord = sampleProbeCoord;
    uint screenProbeIndex = sampleProbeCoord.x + screenspace_probe_stride*sampleProbeCoord.y;
    encodedProbes[i] = sp_loadEncodedProbe(srvInfo.posBuffer, screenProbeIndex);
  }
  return encodedProbes;
}
