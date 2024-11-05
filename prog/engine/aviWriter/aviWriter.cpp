// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_aviWriter.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_localization.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic.h>
#include <perfMon/dag_cpuFreq.h>


#if _TARGET_PC_WIN
#include <windows.h>
#include <vfw.h>
#include <objbase.h>
#pragma comment(lib, "vfw32.lib")

#include <workCycle/dag_wcHooks.h>
#include <workCycle/dag_workCycle.h>
#include "../workCycle/workCyclePriv.h"

static AviWriter *aviWriter = NULL;
static void (*old_dwc_hook_after_frame)() = NULL;
static int old_act_rate = 0, old_acts_per_frame = 0;
static volatile int bytes_written = 0;

static void write_avi_frame()
{
  if (old_dwc_hook_after_frame)
    old_dwc_hook_after_frame();
  aviWriter->onEndOfFrame();
}

bool avi_is_writing_file() { return aviWriter != 0; }

bool avi_start_write(const DataBlock &stg)
{
  if (aviWriter)
  {
    LOGERR_CTX("avi writer already started!");
    return false;
  }

  aviWriter = new AviWriter(stg, NULL);
  if (aviWriter->isInitializedOk())
  {
    old_act_rate = dagor_get_game_act_rate();
    old_acts_per_frame = workcycle_internal::fixedActPerfFrame;
    old_dwc_hook_after_frame = dwc_hook_after_frame;

    dwc_hook_after_frame = write_avi_frame;
    dagor_set_game_act_rate(stg.getInt("act_rate", 60));
    workcycle_internal::fixedActPerfFrame = stg.getInt("acts_per_frame", 2);
    workcycle_internal::curFrameActs = 0;
    return true;
  }
  delete aviWriter;
  aviWriter = NULL;
  return false;
}

bool avi_start_write(const char *dest_folder, int act_rate, int acts_per_frame, const char *codec_str, int codec_quality)
{
  DataBlock stg;
  stg.setStr("folder", dest_folder);
  stg.setStr("codec", codec_str);
  stg.setInt("quality", codec_quality);
  stg.setReal("frameRate", float(act_rate) / float(acts_per_frame));
  stg.setInt("act_rate", act_rate);
  stg.setInt("acts_per_frame", acts_per_frame);
  return avi_start_write(stg);
}

void avi_stop_write()
{
  if (!aviWriter)
  {
    LOGERR_CTX("avi writer hasnt been started!");
    return;
  }

  delete aviWriter;

  dwc_hook_after_frame = old_dwc_hook_after_frame;
  dagor_set_game_act_rate(old_act_rate);
  workcycle_internal::fixedActPerfFrame = old_acts_per_frame;
  workcycle_internal::curFrameActs = 0;
}
#else

bool avi_is_writing_file() { return false; }
bool avi_start_write(const DataBlock &) { return false; }
bool avi_start_write(const char *, int, int, const char *, int) { return false; }
void avi_stop_write() {}
#endif


AviWriter::AviWriter(const DataBlock &blk, ErrorCallback _onErrorCb)
{
  onErrorCb = _onErrorCb;
  initializedOk = initialize(blk);
}

AviWriter::~AviWriter()
{
  if (initializedOk)
    finalize();
}

#if _TARGET_PC_WIN

static bool async = true;

#define CHECK_AVI_RESULT_MSG                                                                                \
  if (result && !SUCCEEDED(res) && onErrorCb)                                                               \
  {                                                                                                         \
    String errorCode;                                                                                       \
    if (res >= AVIERR_UNSUPPORTED && res <= AVIERR_CANTCOMPRESS)                                            \
      errorCode.printf(64, "AviWriter/Error_%d", (res - AVIERR_UNSUPPORTED));                               \
    onErrorCb(String(256, ::get_localized_text("AviWriter/Error"), ::get_localized_text(errorCode.str()))); \
  }

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
#define CHECK_AVI_RESULT(fn)                                           \
  G_ASSERTF(SUCCEEDED(res) || res == AVIERR_FILEWRITE, "res=%x", res); \
  CHECK_AVI_RESULT_MSG                                                 \
  result &= SUCCEEDED(res);                                            \
  if (!SUCCEEDED(res))                                                 \
    DEBUG_CTX("%s %s return error %X", __FUNCTION__, #fn, res);
#else
#define CHECK_AVI_RESULT(fn) \
  CHECK_AVI_RESULT_MSG       \
  result &= SUCCEEDED(res);
#endif

bool AviWriter::initialize(const DataBlock &blk)
{
  bool result = true;
  frameNo = sampleNo = 0;
  bytes_written = 0;
  fileLimit = blk.getInt("fileLimit", 0);

  d3d::get_target_size(screenWidth, screenHeight);


  // File name.

  SYSTEMTIME time;
  GetLocalTime(&time);

  String fileName(1000, "%s/video %04d.%02d.%02d %02d.%02d.%02d.avi", blk.getStr("folder", "."), time.wYear, time.wMonth, time.wDay,
    time.wHour, time.wMinute, time.wSecond);


  // AVI stuff.

  HRESULT res = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  coInitialized = (res == S_OK) || (res == S_FALSE);

  res = AVIFileOpen(&aviFile, fileName, OF_WRITE | OF_CREATE, NULL);
  CHECK_AVI_RESULT(AVIFileOpen);

  switch (res)
  {
    case AVIERR_BADFORMAT:
      debug("The file <%s> couldn't be read, indicating a corrupt file or an unrecognized format.", fileName.str());
      if (coInitialized)
        CoUninitialize();
      return false;
    case AVIERR_MEMORY:
      debug("The file <%s> could not be opened because of insufficient memory.", fileName.str());
      if (coInitialized)
        CoUninitialize();
      return false;
    case AVIERR_FILEREAD:
      debug("A disk error occurred while reading the file <%s>.", fileName.str());
      if (coInitialized)
        CoUninitialize();
      return false;
    case AVIERR_FILEOPEN:
      debug("A disk error occurred while opening the file <%s>.", fileName.str());
      if (coInitialized)
        CoUninitialize();
      return false;
    case REGDB_E_CLASSNOTREG:
      debug("%s%s%s", "According to the registry, the type of file <", fileName.str(),
        "> specified in AVIFileOpen does not have a handler to process it.");
      if (coInitialized)
        CoUninitialize();
      return false;
  }

  if (!SUCCEEDED(res))
  {
    debug("Some unknown error code <%d> returned by AVIFileOpen", (int)res);
    return false;
  }

  // TODO: Process all possible following errors correctly.

  AVISTREAMINFO streamInfo;
  ZeroMemory(&streamInfo, sizeof(AVISTREAMINFO));
  streamInfo.fccType = streamtypeVIDEO;
  streamInfo.fccHandler = 0;
  streamInfo.dwScale = 1;
  streamInfo.dwRate = blk.getReal("frameRate", 30.0f);
  streamInfo.dwSuggestedBufferSize = 0;
  streamInfo.rcFrame.left = 0;
  streamInfo.rcFrame.top = 0;
  streamInfo.rcFrame.right = screenWidth;
  streamInfo.rcFrame.bottom = screenHeight;

  res = AVIFileCreateStream(aviFile, &aviStream, &streamInfo);
  CHECK_AVI_RESULT(AVIFileCreateStream);

  const char *codec = blk.getStr("codec", NULL);
  if (codec && codec[0])
  {
    async = false;
    AVICOMPRESSOPTIONS compressionInfo;
    ZeroMemory(&compressionInfo, sizeof(AVICOMPRESSOPTIONS));
    compressionInfo.fccType = 0;
    compressionInfo.fccHandler = *((unsigned int *)codec);
    compressionInfo.dwQuality = blk.getInt("quality", 100);
    compressionInfo.dwFlags = AVICOMPRESSF_VALID;

    res = AVIMakeCompressedStream(&compressedAVIStream, aviStream, &compressionInfo, NULL);
    CHECK_AVI_RESULT(AVIMakeCompressedStream);
  }
  else
  {
    async = true;
    compressedAVIStream = NULL;
  }

  int stride = POW2_ALIGN(screenWidth * 3, 16);
  BITMAPINFOHEADER streamFormat;
  ZeroMemory(&streamFormat, sizeof(BITMAPINFOHEADER));
  streamFormat.biSize = sizeof(BITMAPINFOHEADER);
  streamFormat.biWidth = screenWidth;
  streamFormat.biHeight = screenHeight;
  streamFormat.biPlanes = 1;
  streamFormat.biBitCount = 24;
  streamFormat.biCompression = BI_RGB;
  streamFormat.biSizeImage = stride * screenHeight;

  res = AVIStreamSetFormat(compressedAVIStream ? compressedAVIStream : aviStream, 0, &streamFormat, sizeof(BITMAPINFOHEADER));
  CHECK_AVI_RESULT(AVIStreamSetFormat);


  // D3D stuff.

  frameBuffer = new unsigned char[stride * screenHeight];
  initializedOk = true;
  for (int i = 0; i < resolvedRenderTargetTexture.size(); ++i)
  {
    resolvedRenderTargetTexture[i] = d3d::create_tex(NULL, screenWidth, screenHeight, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "aviwriter");
    initializedOk &= (resolvedRenderTargetTexture[i] != NULL);
  }

  if (!initializedOk && onErrorCb && false)
    onErrorCb(String(256, ::get_localized_text("AviWriter/Start"), fileName.str()));

  return result;
}

unsigned int AviWriter::getBytesWritten() { return (unsigned int)interlocked_acquire_load(bytes_written); }

class AviAsyncJob : public cpujobs::IJob
{
public:
  Texture *current;
  void *data;
  int stride;
  IAVIStream *aviStream;
  unsigned char *frameBuffer;
  int screenWidth;
  int screenHeight;
  unsigned int frameNo;
  bool result;
  AviWriter::ErrorCallback onErrorCb;
  AviAsyncJob() : current(NULL), data(0), aviStream(0), frameBuffer(0), result(true) {}

  void doJob()
  {
    int dstStride = POW2_ALIGN(screenWidth * 3, 16);
    unsigned char *dstPixel = frameBuffer + (screenHeight - 1) * dstStride;
    unsigned char *srcPixel = (unsigned char *)data;
    unsigned strideDiff = stride - screenWidth * 4;
    for (unsigned int y = 0; y < screenHeight; y++, srcPixel += strideDiff, dstPixel -= screenWidth * 3 + dstStride)
    {
      for (unsigned int x = 0; x < screenWidth; x++, dstPixel += 3, srcPixel += 4)
      {
        dstPixel[0] = srcPixel[0];
        dstPixel[1] = srcPixel[1];
        dstPixel[2] = srcPixel[2];
      }
    }

    LONG bytesWrittenToStream = 0;

    HRESULT res =
      AVIStreamWrite(aviStream, frameNo, 1, frameBuffer, dstStride * screenHeight, AVIIF_KEYFRAME, NULL, &bytesWrittenToStream);
    CHECK_AVI_RESULT(AVIStreamWrite);

    interlocked_add(bytes_written, (int)bytesWrittenToStream);
  }
} asyncJob;


void AviWriter::finalize()
{
  if (frameNo > resolvedRenderTargetTexture.size())
  {
    /*
    //it is easier just to drop last three frames
    for (int i = 0; i < resolvedRenderTargetTexture.size(); ++i)
    {
      captureFrame(frameNo%resolvedRenderTargetTexture.size(), i+1 < resolvedRenderTargetTexture.size());
      frameNo++;
    }*/
    if (async)
    {
      threadpool::wait(&asyncJob);
      if (asyncJob.current)
      {
        asyncJob.current->unlockimg();
        asyncJob.current = NULL;
      }
    }
  }
  for (int i = 0; i < resolvedRenderTargetTexture.size(); ++i)
    del_d3dres(resolvedRenderTargetTexture[i]);

  delete frameBuffer;

  AVIStreamClose(aviStream);
  if (compressedAVIStream)
    AVIStreamClose(compressedAVIStream);
  AVIFileClose(aviFile);
  AVIFileExit();

  if (coInitialized)
    CoUninitialize();
}

bool AviWriter::captureFrame(int lockId)
{
  void *data;
  int stride;
  bool result = true;

  if (async)
  {
    threadpool::wait(&asyncJob);
    if (asyncJob.current)
    {
      asyncJob.current->unlockimg();
      asyncJob.current = NULL;
    }
  }
  if (resolvedRenderTargetTexture[lockId]->lockimg(&data, stride, 0, TEXLOCK_READ))
  {
    asyncJob.onErrorCb = onErrorCb;
    asyncJob.frameBuffer = frameBuffer;
    asyncJob.screenWidth = screenWidth;
    asyncJob.screenHeight = screenHeight;
    asyncJob.frameNo = sampleNo;
    asyncJob.data = data;
    asyncJob.stride = stride;
    asyncJob.aviStream = compressedAVIStream ? compressedAVIStream : aviStream;
    asyncJob.current = resolvedRenderTargetTexture[lockId];
    asyncJob.result = true;
    if (async)
      threadpool::add(&asyncJob);
    else
    {
      asyncJob.doJob();
      resolvedRenderTargetTexture[lockId]->unlockimg();
    }
    sampleNo++;
  }
  return result;
}

bool AviWriter::onEndOfFrame()
{
  if (async)
    threadpool::wait(&asyncJob);

  bool result = true;
  Driver3dRenderTarget rt;
  d3d::get_render_target(rt);

  int lockId = frameNo % resolvedRenderTargetTexture.size();
  if (frameNo >= resolvedRenderTargetTexture.size())
    result = captureFrame(lockId);

  int captureId = async ? (lockId + resolvedRenderTargetTexture.size() - 1) % resolvedRenderTargetTexture.size() : lockId;
  d3d::stretch_rect(rt.getColor(0).tex, resolvedRenderTargetTexture[captureId]);
  int stride;
  if (resolvedRenderTargetTexture[captureId]->lockimg(NULL, stride, 0, TEXLOCK_READ))
    resolvedRenderTargetTexture[captureId]->unlockimg();
  frameNo++;
  return result;
}
#undef CHECK_AVI_RESULT
#undef CHECK_AVI_RESULT_MSG

#else

bool AviWriter::captureFrame(int) { return false; }
bool AviWriter::initialize(const DataBlock &) { return false; }
unsigned int AviWriter::getBytesWritten() { return 0; }

void AviWriter::finalize() {}
bool AviWriter::onEndOfFrame() { return false; }
#endif
