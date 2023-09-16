//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class Compression
{
public:
  // the default implementation does nothing
  struct CompressionEntry
  {
    const char *name;
    const Compression *inst;
    char id; //-V707
  };

  Compression() {}
  virtual ~Compression() {}

  static const Compression &getInstanceByName(const char *name);
  static const Compression &getInstanceById(int id);
  static const Compression &getBestCompression();
  static const CompressionEntry *getCompressionTypes();
  static const Compression *selectCompressionByList(const char *list);

  static int getCompressionTypesCount();
  int getRequiredCompressionBufferLength(int plainDataLen) const;
  virtual int getRequiredCompressionBufferLength(int plainDataLen, int level) const;
  virtual int getRequiredDecompressionBufferLength(const void *data_, int dataLen) const;
  virtual bool isValid() const { return false; }
  virtual char getId() const { return 'i'; }
  virtual const char *getName() const;
  virtual int getMinLevel() const { return 0; }
  virtual int getMaxLevel() const { return 0; }
  int getDefaultLevel() const { return defaultLevel; }
  int clampLevel(int level) const;

  const char *compress(const void *in_, int inLen, void *out_, int &outLen) const;

  virtual const char *compress(const void *in_, int inLen, void *out_, int &outLen, int level) const;

  virtual const char *decompress(const void *in_, int inLen, void *out_, int &outLen) const;

  bool compressWithId(const void *in_, int inLen, void *out_, int &outLen) const;

  bool compressWithId(const void *in_, int inLen, void *out_, int &outLen, int level) const;

protected:
  // LZ4 claims to support ~1.9Gb blocks.
  // 1Gb is an insane data block anyways and should be more than enough.
  static constexpr int MAX_PLAIN_DATA_SIZE = 1 << 30;

  int defaultLevel = 0;
};


inline int Compression::getRequiredCompressionBufferLength(int plainDataLen) const
{
  return getRequiredCompressionBufferLength(plainDataLen, defaultLevel);
}


inline const char *Compression::compress(const void *in_, int inLen, void *out_, int &outLen) const
{
  return compress(in_, inLen, out_, outLen, defaultLevel);
}


inline bool Compression::compressWithId(const void *in_, int inLen, void *out_, int &outLen) const
{
  return compressWithId(in_, inLen, out_, outLen, defaultLevel);
}
