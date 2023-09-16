//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "compression.h"
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_simpleString.h>

class VromfsCompression : public Compression
{
public:
  enum InputDataType
  {
    IDT_VROM_PLAIN,
    IDT_VROM_COMPRESSED,
    IDT_DATA
  };

  enum CheckType
  {
    CHECK_LAZY,
    CHECK_STRICT,
    CHECK_VERBOSE
  };

  VromfsCompression();
  VromfsCompression(verify_signature_cb callback, const char *fname);

  int getRequiredCompressionBufferLength(int plainDataLen, int level) const override;
  int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const override;
  int getMinLevel() const override;
  int getMaxLevel() const override;
  bool isValid() const override { return true; }
  char getId() const override { return 'V'; }
  const char *getName() const override;
  const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const override;
  const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const override;

  void setVerifySignatureCallback(verify_signature_cb callback) { verifySignatureCb = callback; }

  void setVromFileName(const char *fname) { fileName = fname; }

  InputDataType getDataType(const char *data, int size, CheckType checkType) const;
  bool extractVersion(const char *data, int size, unsigned &version) const;

private:
  verify_signature_cb verifySignatureCb;
  SimpleString fileName;
};
