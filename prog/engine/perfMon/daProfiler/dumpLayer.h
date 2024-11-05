// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

// almost direct copy of optick layer
namespace da_profiler
{

enum
{
  TYPE_CPU = 0,
  TYPE_GPU,
  TYPES_COUNT,
  TYPE_NONE = -1
};

struct ThreadMask
{
  enum Type
  {
    None = 0,
    Main = 1 << 0,
    GPU = 1 << 1,
    IO = 1 << 2,
    Idle = 1 << 3,
    Render = 1 << 4,
  };
};

struct DumpHeader
{
  static const uint32_t MAGIC = 0xC50FC50Fu;
  static const uint16_t VERSION = 0;
  enum Flags : uint16_t
  {
    Reserved0 = 1 << 0,
    Zlib = 1 << 1,
  };
  uint32_t magic = MAGIC;
  uint16_t version = VERSION;
  uint16_t flags = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint32_t NETWORK_PROTOCOL_VERSION = 1;
static const uint16_t NETWORK_APPLICATION_ID = 0xC50F;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct DataResponse
{
  enum Type : uint16_t
  {
    FrameDescriptionBoard = 0, // DescriptionBoard for Instrumental Frames
    EventFrame = 1,            // Instrumental Data
    Settings = 2,              // Current settings
    NullFrame = 3,             // Last Fame Mark
    ReportProgress = 4,        // Report Current Progress
    Handshake = 5,             // Handshake Response
    UniqueName = 6,
    SynchronizationData = 7,       // Synchronization Data for the thread
    TagsPack = 8,                  // Pack of tags
    CallstackDescriptionBoard = 9, // DescriptionBoard with resolved function addresses
    CallstackPack = 10,            // Pack of CallStacks
    SettingsPack = 11,
    Heartbeat = 12,
    ReportLiveFrameTime = 13,
    Plugins = 14,           // pack of current plugins
    UniqueEventsBoard = 15, // Pack of UniqueEvents
    //...
    Reserved_255 = 255,

    FiberSynchronizationData = 1 << 8, // Synchronization Data for the Fibers
    SyscallPack,
    SummaryPack,
    FramesPack,
  };

  uint32_t version;
  uint32_t size;
  Type type;
  uint16_t application;

  DataResponse(Type t, uint32_t s) : version(NETWORK_PROTOCOL_VERSION), size(s), type(t), application(NETWORK_APPLICATION_ID) {}
  bool isValid() const { return application == NETWORK_APPLICATION_ID; }
};

template <class IGenSaveCB>
inline void send_data(IGenSaveCB &cb, DataResponse::Type type, const void *data, size_t size)
{
  DataResponse response(type, size);

  cb.write(&response, sizeof(response));
  cb.write(data, size);
}

template <class IGenSaveCB, class Stream>
inline void send_data(IGenSaveCB &cb, DataResponse::Type type, const Stream &stream)
{
  send_data(cb, type, stream.data(), stream.size());
}

enum class ReadResponseCbStatus
{
  Skip,
  Continue,
  Stop
}; // if Continue - all data has been read from source stream, if Skip - we need to skip it ourselves
enum class ReadResponseStatus
{
  Ok,
  Eof,
  Stop
};

template <class IGenLoadCB, class DataResponseCB>
inline ReadResponseStatus read_responses(IGenLoadCB &load_cb, const DataResponseCB &resp_cb)
{
  auto skip = [&](size_t sz) {
    char buf[256];
    while (sz > 0)
    {
      const size_t readSz = sz < sizeof(buf) ? sz : sizeof(buf);
      if (load_cb.tryRead(buf, readSz) != readSz)
        return false;
      sz -= readSz;
    }
    return true;
  };

  DataResponse resp(DataResponse::Reserved_255, 0);
  while (load_cb.tryRead(&resp, sizeof(resp)) == sizeof(resp))
  {
    ReadResponseCbStatus ret = resp_cb(load_cb, resp);
    if (ret == ReadResponseCbStatus::Stop)
      return ReadResponseStatus::Stop;
    if (ret == ReadResponseCbStatus::Skip)
    {
      if (!skip(resp.size))
        return ReadResponseStatus::Eof;
    }
  }
  return ReadResponseStatus::Ok;
}


} // namespace da_profiler
