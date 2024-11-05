// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dataHelpers.h"

#include <util/dag_string.h>
#include <util/dag_stdint.h>
#include <debug/dag_log.h>
#include <json/json.h>

#include <eventLog/eventLog.h>

namespace event_log
{
Packet::Packet(const char *type, Json::Value *meta, const void *data, size_t size, const Json::Value &default_meta)
{
  G_ASSERT(type && *type);

  Json::Value jsheader = default_meta;
  if (meta)
  {
    G_ASSERTF(!meta->isMember("type"), "Field meta.type will be overwritten by type argument. So passing it is redundant");
    for (const eastl::string &key : meta->getMemberNames())
      jsheader[key] = (*meta)[key];
  }

  jsheader["type"] = type;


  Json::FastWriter writer;
  eastl::string header = writer.write(jsheader);

  uint32_t header_len = (uint32_t)header.length();
  clear_and_resize(this->buffer, sizeof(header_len) + header_len + size);
  memcpy(this->buffer.data(), (const uint8_t *)&header_len, sizeof(header_len));
  memcpy(this->buffer.data() + sizeof(header_len), header.c_str(), header_len);
  memcpy(this->buffer.data() + sizeof(header_len) + header_len, data, size);
}


const char *Packet::data() const { return (const char *)buffer.data(); }


uint32_t Packet::size() const { return data_size(buffer); }

} // namespace event_log
