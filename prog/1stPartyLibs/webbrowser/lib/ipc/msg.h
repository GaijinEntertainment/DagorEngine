#pragma once

#include "msg/anymessage_generated.h"

#define __SEND_START(type) \
{ \
  using namespace ipc::msg; \
  flatbuffers::FlatBufferBuilder fbb;

#define __SEND_END(type) \
  auto msg = CreateAnyMessage(fbb, 0, Payload::##type, p.Union()); \
  fbb.Finish(msg); \
  if (!this->channel.send(fbb.GetBufferPointer(), fbb.GetSize())) \
    WRN("%s: Could not send %lu bytes of %s", __FUNCTION__, fbb.GetSize(), EnumNamePayload(Payload::##type)); \
}

#define __SEND_FAIL_WRN_THRESH 60
#define __SEND_TOTAL_WRN_THRESH 1000

#define __SEND_END_SILENT(type) \
  static uint64_t total##type = 0; \
  static uint64_t failed##type = 0; \
  auto msg = CreateAnyMessage(fbb, 0, Payload::##type, p.Union()); \
  fbb.Finish(msg); \
  total##type++; \
  if (!this->channel.send(fbb.GetBufferPointer(), fbb.GetSize())) \
    failed##type++; \
  if (failed##type > __SEND_FAIL_WRN_THRESH || (failed##type > 0 && total##type > __SEND_TOTAL_WRN_THRESH)) \
  { \
    WRN("%s: failed send threshold reached for %s: %llu failed of %llu total, %g%%", \
        __FUNCTION__, EnumNamePayload(Payload::##type), failed##type, total##type, ((float)failed##type/total##type)); \
    total##type = 0; \
    failed##type = 0; \
  } \
}

#define SEND_SIMPLE_MSG(type) \
  __SEND_START(type) \
  auto p = Create##type(fbb); \
  __SEND_END(type)

#define SEND_DIRECT_MSG(type, ...) \
  __SEND_START(type) \
  auto p = Create##type##Direct(fbb, __VA_ARGS__); \
  __SEND_END(type)

#define SEND_DATA_MSG(type, ...) \
  __SEND_START(type) \
  auto p = Create##type(fbb, __VA_ARGS__); \
  __SEND_END(type)

#define SEND_DATA_MSG_SILENT(type, ...) \
  __SEND_START(type) \
  auto p = Create##type(fbb, __VA_ARGS__); \
  __SEND_END_SILENT(type)
