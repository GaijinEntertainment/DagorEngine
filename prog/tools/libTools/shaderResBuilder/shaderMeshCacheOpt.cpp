// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <shaders/dag_shaders.h>
#include "cacheOpt.h"

// optimize data for cache
void ShaderMeshData::optimizeForCache(bool opt_overdraw_too)
{
  for (int i = 0; i < elems.size(); ++i)
    if (!elems[i].optDone)
    {
      optimizeForCache(elems[i], opt_overdraw_too);
      elems[i].optDone = 1;
    }
}

void ShaderMeshData::optimizeForCache(RElem &re, bool opt_overdraw_too)
{
  GlobalVertexDataSrc &vd = *re.vertexData;
  Tab<float> unpacked_vert_f3;

  int numv = re.numv;
  int numf = re.numf;

  if (re.si == RELEM_NO_INDEX_BUFFER)
    return;

  int pos_offset = -1, pos_type = -1, coffset = 0;
  if (opt_overdraw_too)
    for (auto &c : vd.vDesc)
    {
      if (c.streamId != 0)
        continue;
      unsigned chSize = 0;
      if (c.vbu == SCUSAGE_POS && c.vbui == 0)
      {
        pos_offset = coffset;
        pos_type = c.t;
      }
      if (channel_size(c.t, chSize))
        coffset += chSize;
    }

  float *vd_f3 = nullptr;
  int vd_stride_f3 = 0;
  switch (pos_type)
  {
    case SCTYPE_FLOAT3:
      vd_f3 = (float *)&vd.vData[re.sv * vd.stride + pos_offset];
      vd_stride_f3 = vd.stride;
      break;

    case SCTYPE_SHORT4:
      unpacked_vert_f3.resize(numv * 3);
      vd_f3 = unpacked_vert_f3.data();
      for (const auto *s = &vd.vData[re.sv * vd.stride + pos_offset], *se = s + numv * vd.stride; s < se; s += vd.stride, vd_f3 += 3)
        vd_f3[0] = *(int16_t *)(s + 0), vd_f3[1] = *(int16_t *)(s + 2), vd_f3[2] = *(int16_t *)(s + 4);
      vd_f3 = unpacked_vert_f3.data();
      vd_stride_f3 = elem_size(unpacked_vert_f3) * 3;
      break;

    case SCTYPE_HALF4:
      unpacked_vert_f3.resize(numv * 3);
      vd_f3 = unpacked_vert_f3.data();
      for (const auto *s = &vd.vData[re.sv * vd.stride + pos_offset], *se = s + numv * vd.stride; s < se; s += vd.stride, vd_f3 += 3)
        vd_f3[0] = half_to_float(*(uint16_t *)(s + 0)), vd_f3[1] = half_to_float(*(uint16_t *)(s + 2)),
        vd_f3[2] = half_to_float(*(uint16_t *)(s + 4));
      vd_f3 = unpacked_vert_f3.data();
      vd_stride_f3 = elem_size(unpacked_vert_f3) * 3;
      break;

    default:
      if (opt_overdraw_too)
        logwarn("skip opt4cache/overdraw: pos_offset=%d pos_type=0x%x", pos_offset, pos_type);
      break;
  }

  if (vd.iData.size())
  {
    G_VERIFY(
      optimize_list_4cache(&vd.vData[re.sv * vd.stride], vd.stride, re.sv, numv, &vd.iData[re.si], numf, 24, vd_f3, vd_stride_f3));
  }
  else
  {
    G_VERIFY(optimize_list_4cache(&vd.vData[re.sv * vd.stride], vd.stride, re.sv, numv, (unsigned int *)&vd.iData32[re.si], numf, 24,
      vd_f3, vd_stride_f3));
  }
}
