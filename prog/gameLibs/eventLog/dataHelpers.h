#pragma once

#include <generic/dag_smallTab.h>
#include <util/dag_stdint.h>

// Forward declarations
namespace Json
{
class Value;
}


namespace event_log
{
struct Packet
{
  Packet(const char *type, Json::Value *meta, const void *data, size_t size, const Json::Value &default_meta);

  const char *data() const;
  uint32_t size() const;

  SmallTab<uint8_t, TmpmemAlloc> buffer;
}; // struct Packet

} // namespace event_log
