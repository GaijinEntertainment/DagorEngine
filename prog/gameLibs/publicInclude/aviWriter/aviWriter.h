//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
#if _TARGET_PC_WIN
struct IAVIFile;
struct IAVIStream;
class BaseTexture;
typedef BaseTexture Texture;
#include <generic/dag_carray.h>
#endif


//! start AVI writer; settings are read from DataBlock; return false on error
bool avi_start_write(const DataBlock &stg);

//! return true when AVI writer is started and writing file
bool avi_is_writing_file();

//! stop AVI writer and deallocate resources
void avi_stop_write();


#ifdef __cplusplus
class AviWriter
{
  friend class AviAsyncJob;
  typedef void (*ErrorCallback)(const char *);

public:
  unsigned int fileLimit;

  AviWriter(const DataBlock &blk, ErrorCallback _onErrorCb);
  ~AviWriter();

  bool onEndOfFrame();
  bool initialize(const DataBlock &blk);
  void finalize();
  unsigned int getBytesWritten();
  bool isInitializedOk() { return initializedOk; };

protected:
  bool captureFrame(int lockId);
  int screenWidth;
  int screenHeight;
  unsigned int frameNo, sampleNo;
  bool initializedOk;
  ErrorCallback onErrorCb;

#if _TARGET_PC_WIN
  bool coInitialized;
  IAVIFile *aviFile;
  IAVIStream *aviStream;
  IAVIStream *compressedAVIStream;
  unsigned char *frameBuffer;
  carray<Texture *, 3> resolvedRenderTargetTexture;
#endif
};
#endif
