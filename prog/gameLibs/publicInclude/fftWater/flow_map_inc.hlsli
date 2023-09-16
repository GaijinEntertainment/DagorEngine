#pragma once

#define MAX_FLOWMAP_WINDS 16

struct FlowmapWind
{
  float4 area;
  float2 dir;
  float padding0;
  float padding1;
};
