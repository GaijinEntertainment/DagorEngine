// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/shadersMetaData/dx11/unpack.h>

bool dx11::decode_metadata(dag::ConstSpan<uint8_t> metadata, DecodedShader &out)
{
  if (metadata.size() < SIMPLE_METADATA_SIZE)
    return false;

  auto &simple = *reinterpret_cast<const SimpleHeader *>(metadata.data());
  out.totalBytecodeSize = simple.bytecodeByteSize;
  out.maxConstantRegUsed = simple.maxConstantRegUsed;
  out.maxRtvUsed = simple.maxRtvUsed;
  out.uavReadMask = get_uav_read_mask(simple);
  out.uavWriteMask = get_uav_write_mask(simple);

  if (metadata.size() >= COMBINED_METADATA_SIZE)
  {
    auto &combi = *reinterpret_cast<const CombinedHeader *>(metadata.data() + COMBINED_HEADER_OFFSET);
    if (combi.magic == COMBINED_SHADERS_IDENT)
    {
      out.combined = true;
      out.hsTopology = combi.hsLenDwordsAndTopology >> HS_TOPOLOGY_SHIFT;
      uint32_t vsLen = combi.vsLenDwords * 4;
      uint32_t hsLen = (combi.hsLenDwordsAndTopology & COMBINED_LEN_MASK) * 4;
      uint32_t dsLen = combi.dsLenDwords * 4;
      uint32_t gsLen = combi.gsLenDwords * 4;
      out.vs = {0, vsLen};
      out.hs = {vsLen, hsLen};
      out.ds = {vsLen + hsLen, dsLen};
      out.gs = {vsLen + hsLen + dsLen, gsLen};
      return vsLen + hsLen + dsLen + gsLen <= out.totalBytecodeSize;
    }
  }

  out.vs = {0, out.totalBytecodeSize};
  return true;
}
