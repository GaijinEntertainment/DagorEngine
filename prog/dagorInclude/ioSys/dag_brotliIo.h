//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_memIo.h>

typedef struct BrotliDecoderStateStruct *BrotliDecoderStateCtx;

struct BrotliStreamDecompress : public IStreamDecompress
{
  BrotliStreamDecompress();
  ~BrotliStreamDecompress();
  virtual StreamDecompressResult decompress(dag::ConstSpan<uint8_t> in, Tab<char> &out, size_t *nbytes_read = nullptr) override;

private:
  static constexpr int TEMP_BUFFER_SIZE = 16384;
  uint8_t tmpBuffer[TEMP_BUFFER_SIZE];
  BrotliDecoderStateCtx state;
};
