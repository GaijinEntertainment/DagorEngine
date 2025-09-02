//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/aot.h>
#include <compressionUtils/compression.h>
#include <daNet/bitStream.h>
#include <dasModules/dasManagedTab.h>
#include <memory/dag_framemem.h>

const int compressionTypeByteSize = 1;

namespace bind_dascript
{
inline void das_compress_data(const char *compression_name, das::TArray<uint8_t> &out,
  const das::TBlock<void, danet::BitStream &> &block, das::Context *context, das::LineInfoArg *at)
{
  danet::BitStream tmpBs(framemem_ptr());
  vec4f arg = das::cast<danet::BitStream &>::from(tmpBs);
  context->invoke(block, &arg, nullptr, at);

  int inSize = tmpBs.GetNumberOfBytesUsed();
  uint8_t *inData = tmpBs.GetData();

  if (inData == nullptr || inSize == 0)
    return; // nothing to compress

  const Compression &compression =
    compression_name ? Compression::getInstanceByName(compression_name) : Compression::getBestCompression();
  if (!compression.isValid())
  {
    logerr("Wrong compression name = <%s>", compression_name);
    return;
  }
  int reqSize = compression.getRequiredCompressionBufferLength(inSize);
  das::builtin_array_resize(out, reqSize + compressionTypeByteSize, 1, context, at);
  out.data[0] = compression.getId();
  compression.compress(inData, inSize, out.data + compressionTypeByteSize, reqSize);
  das::builtin_array_resize(out, reqSize + compressionTypeByteSize, 1, context, at);
}

inline void das_decompress_data(const das::TArray<uint8_t> &in, const das::TBlock<void, const danet::BitStream &> &block,
  das::Context *context, das::LineInfoArg *at)
{
  if (in.data == nullptr || in.size == 0)
    return; // nothing to decompress

  int inSize = in.size - compressionTypeByteSize;
  char *inData = in.data + compressionTypeByteSize;
  char compressionId = in.data[0];

  const Compression &compression = Compression::getInstanceById(compressionId);
  if (!compression.isValid())
  {
    logerr("Broken compressed data. Wrong compression id=<%c> in header", compressionId);
    return;
  }
  int reqSize = compression.getRequiredDecompressionBufferLength(inData, inSize);
  Tab<uint8_t> out(framemem_ptr());
  out.resize(reqSize);
  compression.decompress(inData, inSize, out.data(), reqSize);

  danet::BitStream tmpBs(out.data(), reqSize, false, framemem_ptr());
  vec4f arg = das::cast<danet::BitStream &>::from(tmpBs);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript