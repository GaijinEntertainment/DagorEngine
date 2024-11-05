// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "render.h"

namespace drv3d_metal
{
  VDecl::VDecl(VSDTYPE* d)
  {
    vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    [vertexDescriptor retain];

    int index = 0;
    int offset = 0;
    int layout = -1;
    bool per_instance = false;

    num_location = 0;
    num_streams = 0;

    hash = 0;

    if (*d == VSD_END)
      return;

    for (; *d != VSD_END; ++d)
    {
      if ((*d&VSDOP_MASK) == VSDOP_INPUT)
      {
        if (*d&VSD_SKIPFLG)
        {
          offset += GET_VSDSKIP(*d) * 4;
          continue;
        }

        MTLVertexFormat fmt = MTLVertexFormatInvalid;
        int sz = 0;

        switch (*d&VSDT_MASK)
        {
          case VSDT_FLOAT1:   fmt = MTLVertexFormatFloat;   sz = 4; break;
          case VSDT_FLOAT2:   fmt = MTLVertexFormatFloat2;   sz = 8; break;
          case VSDT_FLOAT3:   fmt = MTLVertexFormatFloat3;   sz = 12; break;
          case VSDT_FLOAT4:   fmt = MTLVertexFormatFloat4;   sz = 16; break;
          case VSDT_INT1:     fmt = MTLVertexFormatInt;   sz = 4; break;
          case VSDT_INT2:     fmt = MTLVertexFormatInt2;   sz = 8; break;
          case VSDT_INT3:     fmt = MTLVertexFormatInt3;   sz = 12; break;
          case VSDT_INT4:     fmt = MTLVertexFormatInt4;   sz = 16; break;
          case VSDT_UINT1:     fmt = MTLVertexFormatUInt;   sz = 4; break;
          case VSDT_UINT2:     fmt = MTLVertexFormatUInt2;   sz = 8; break;
          case VSDT_UINT3:     fmt = MTLVertexFormatUInt3;   sz = 12; break;
          case VSDT_UINT4:     fmt = MTLVertexFormatUInt4;   sz = 16; break;

          case VSDT_HALF2:  fmt = MTLVertexFormatHalf2; sz = 4; break;
          case VSDT_SHORT2N:  fmt = MTLVertexFormatShort2Normalized;  sz = 4; break;
          case VSDT_USHORT2N: fmt = MTLVertexFormatUShort2Normalized; sz = 4; break;
          case VSDT_SHORT2:   fmt = MTLVertexFormatShort2;   sz = 4; break;

          case VSDT_HALF4:  fmt = MTLVertexFormatHalf4; sz = 8; break;
          case VSDT_SHORT4N:  fmt = MTLVertexFormatShort4Normalized; sz = 8; break;
          case VSDT_USHORT4N: fmt = MTLVertexFormatUShort4Normalized; sz = 8; break;
          case VSDT_SHORT4:   fmt = MTLVertexFormatShort4;   sz = 8; break;

          case VSDT_UDEC3:  /*t = D3DDECLTYPE_UDEC3;*/  sz = 4; break;
          case VSDT_DEC3N:  /*t = D3DDECLTYPE_DEC3N;*/  sz = 4; break;

          case VSDT_E3DCOLOR: fmt = MTLVertexFormatUChar4Normalized; sz = 4; break;
          case VSDT_UBYTE4:   fmt = MTLVertexFormatUChar4;   sz = 4; break;
        }

        vertexDescriptor.attributes[index].format = fmt;
        vertexDescriptor.attributes[index].bufferIndex = layout;
        vertexDescriptor.attributes[index].offset = offset;

        locations[num_location].offset = offset;
        locations[num_location].size = sz;
        locations[num_location].stream = layout;
        locations[num_location].vsdr = (*d&VSDR_MASK);

        hash_combine(hash, std::hash<uint32_t>{}(fmt));
        hash_combine(hash, std::hash<uint32_t>{}(layout));
        hash_combine(hash, std::hash<uint32_t>{}(offset));
        hash_combine(hash, std::hash<uint32_t>{}(sz));
        hash_combine(hash, std::hash<uint32_t>{}((*d&VSDR_MASK)));

        num_location++;

        offset += sz;
        index++;
      }
      else
      if ((*d&VSDOP_MASK) == VSDOP_STREAM)
      {
        if (layout != -1)
        {
          vertexDescriptor.layouts[layout].stride = offset;
          vertexDescriptor.layouts[layout].stepFunction =
            per_instance ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
        }

        layout = GET_VSDSTREAM(*d);
        per_instance = *d & VSDS_PER_INSTANCE_DATA;
        offset = 0;

        if (per_instance)
          instanced_mask |= 1 << layout;

        hash_combine(hash, std::hash<uint32_t>{}(layout));
        hash_combine(hash, std::hash<uint32_t>{}(per_instance));

        num_streams++;
      }
    }

    vertexDescriptor.layouts[layout].stride = offset;
    vertexDescriptor.layouts[layout].stepFunction = per_instance ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
  }

  void VDecl::release()
  {
    [vertexDescriptor release];
    delete this;
  }
}
