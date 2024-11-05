// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <smolv.h>

#include <generic/dag_sort.h>
#include <EASTL/optional.h>
#include "pipeline_description.h"

using namespace drv3d_vulkan;

static int attrib_sorter(const VertexAttributeDesc *l, const VertexAttributeDesc *r) { return (int)l->location - (int)r->location; }

static int channel_remap(int reg)
{
  static const char remap[] = //
    {
      0,  // VSDR_POS        0
      1,  // VSDR_BLENDW     1
      1,  // VSDR_BLENDIND   2
      2,  // VSDR_NORM       3
      5,  // VSDR_PSIZE      4
      3,  // VSDR_DIFF       5
      4,  // VSDR_SPEC       6
      8,  // VSDR_TEXC0      7
      9,  // VSDR_TEXC1      8
      10, // VSDR_TEXC2      9
      11, // VSDR_TEXC3      10
      12, // VSDR_TEXC4      11
      13, // VSDR_TEXC5      12
      14, // VSDR_TEXC6      13
      15, // VSDR_TEXC7      14
      6,  // VSDR_POS2       15
      7,  // VSDR_NORM2      16
      5,  // VSDR_TEXC15     17
      5,  // VSDR_TEXC8      18
      4,  // VSDR_TEXC9      19
      14, // VSDR_TEXC10     20
      15, // VSDR_TEXC11     21
      1,  // VSDR_TEXC12     22
      6,  // VSDR_TEXC13     23
      7,  // VSDR_TEXC14     24
    };
  reg = remap[reg];
  return reg;
}

void InputLayout::fromVdecl(const VSDTYPE *decl)
{
  const VSDTYPE *__restrict vt = decl;
  uint32_t ofs = 0;
  streams = InputStreamSet(0);
  mem_set_0(attribs);
  uint32_t streamIndex = 0;
  uint32_t attribCount = 0;
  // ensure the mapping is ok
  G_STATIC_ASSERT(VSDT_FLOAT2 >> 16 == 1);

  for (; *vt != VSD_END; ++vt)
  {
    if ((*vt & VSDOP_MASK) == VSDOP_INPUT)
    {
      if (*vt & VSD_SKIPFLG)
      {
        ofs += GET_VSDSKIP(*vt) * 4;
        continue;
      }

      VertexAttributeDesc layout;
      layout.used = 1;
      layout.location = channel_remap(GET_VSDREG(*vt));
      layout.binding = streamIndex;
      layout.offset = ofs;

      uint32_t sz = 0; // size of entry

      switch (*vt & VSDT_MASK)
      {
        case VSDT_FLOAT1:
          layout.formatIndex = VK_FORMAT_R32_SFLOAT;
          sz = 32;
          break;
        case VSDT_FLOAT2:
          layout.formatIndex = VK_FORMAT_R32G32_SFLOAT;
          sz = 32 + 32;
          break;
        case VSDT_FLOAT3:
          layout.formatIndex = VK_FORMAT_R32G32B32_SFLOAT;
          sz = 32 + 32 + 32;
          break;
        case VSDT_FLOAT4:
          layout.formatIndex = VK_FORMAT_R32G32B32A32_SFLOAT;
          sz = 32 + 32 + 32 + 32;
          break;

        case VSDT_INT1:
          layout.formatIndex = VK_FORMAT_R32_SINT;
          sz = 32;
          break;
        case VSDT_INT2:
          layout.formatIndex = VK_FORMAT_R32G32_SINT;
          sz = 32 + 32;
          break;
        case VSDT_INT3:
          layout.formatIndex = VK_FORMAT_R32G32B32_SINT;
          sz = 32 + 32 + 32;
          break;
        case VSDT_INT4:
          layout.formatIndex = VK_FORMAT_R32G32B32A32_SINT;
          sz = 32 + 32 + 32 + 32;
          break;

        case VSDT_UINT1:
          layout.formatIndex = VK_FORMAT_R32_UINT;
          sz = 32;
          break;
        case VSDT_UINT2:
          layout.formatIndex = VK_FORMAT_R32G32_UINT;
          sz = 32 + 32;
          break;
        case VSDT_UINT3:
          layout.formatIndex = VK_FORMAT_R32G32B32_UINT;
          sz = 32 + 32 + 32;
          break;
        case VSDT_UINT4:
          layout.formatIndex = VK_FORMAT_R32G32B32A32_UINT;
          sz = 32 + 32 + 32 + 32;
          break;

        case VSDT_HALF2:
          layout.formatIndex = VK_FORMAT_R16G16_SFLOAT;
          sz = 16 + 16;
          break;
        case VSDT_SHORT2N:
          layout.formatIndex = VK_FORMAT_R16G16_SNORM;
          sz = 16 + 16;
          break;
        case VSDT_SHORT2:
          layout.formatIndex = VK_FORMAT_R16G16_SINT;
          sz = 16 + 16;
          break;
        case VSDT_USHORT2N:
          layout.formatIndex = VK_FORMAT_R16G16_UNORM;
          sz = 16 + 16;
          break;

        case VSDT_HALF4:
          layout.formatIndex = VK_FORMAT_R16G16B16A16_SFLOAT;
          sz = 16 + 16 + 16 + 16;
          break;
        case VSDT_SHORT4N:
          layout.formatIndex = VK_FORMAT_R16G16B16A16_SNORM;
          sz = 16 + 16 + 16 + 16;
          break;
        case VSDT_SHORT4:
          layout.formatIndex = VK_FORMAT_R16G16B16A16_SINT;
          sz = 16 + 16 + 16 + 16;
          break;
        case VSDT_USHORT4N:
          layout.formatIndex = VK_FORMAT_R16G16B16A16_UNORM;
          sz = 16 + 16 + 16 + 16;
          break;

        case VSDT_UDEC3:
          layout.formatIndex = VK_FORMAT_A2B10G10R10_UINT_PACK32;
          sz = 10 + 10 + 10 + 2;
          break;
        case VSDT_DEC3N:
          layout.formatIndex = VK_FORMAT_A2B10G10R10_SNORM_PACK32;
          sz = 10 + 10 + 10 + 2;
          break;

        case VSDT_E3DCOLOR:
          layout.formatIndex = E3DCOLOR_FORMAT;
          sz = 8 + 8 + 8 + 8;
          break;
        case VSDT_UBYTE4:
          layout.formatIndex = VK_FORMAT_R8G8B8A8_UINT;
          sz = 8 + 8 + 8 + 8;
          break;
        default: G_ASSERTF(false, "invalid vertex declaration type"); break;
      }
      attribs[attribCount++] = layout;
      ofs += sz / 8;
    }
    else if ((*vt & VSDOP_MASK) == VSDOP_STREAM)
    {
      streamIndex = GET_VSDSTREAM(*vt);
      ofs = 0;
      streams.used[streamIndex] = 1;
      if (*vt & VSDS_PER_INSTANCE_DATA)
        streams.stepRate[streamIndex] = VK_VERTEX_INPUT_RATE_INSTANCE;
      else
        streams.stepRate[streamIndex] = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    else
    {
      G_ASSERTF(0, "Invalid vsd opcode 0x%08X", *vt);
    }
  }

  // needs to be sorted to allow reuse of layouts
  sort(attribs, &attrib_sorter);
}

bool drv3d_vulkan::InputLayout::isSame(const CreationInfo &info) { return *this == info; }
