// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerMessageServer.h"
// #include "daProfilePlatform.h"
#include "stl/daProfilerVector.h"
#include <mutex>

// message server. Receives message from daProfiler GUI tool
namespace da_profiler
{

class ProfilerMessageServer
{
  vector<read_and_apply_message_cb_t> messages;
  mutable std::recursive_mutex messageLock;

public:
  ~ProfilerMessageServer() { shutdown(); }
  void addMessage(uint16_t type, read_and_apply_message_cb_t cb)
  {
    if (type == uint16_t(~uint16_t(0)))
      return;
    std::lock_guard<std::recursive_mutex> lock(messageLock);
    if (type >= messages.size())
      messages.resize(type + 1);
    messages[type] = cb;
  }
  bool perform(uint16_t type, uint32_t len, IGenLoad &lcb, IGenSave &scb)
  {
    read_and_apply_message_cb_t message = NULL;
    {
      std::lock_guard<std::recursive_mutex> lock(messageLock);
      if (type < messages.size())
        message = messages[type];
    }
    if (!message)
      return false;
    message(type, len, lcb, scb);
    return true;
  }

  void shutdown()
  {
    std::lock_guard<std::recursive_mutex> lock(messageLock);
    messages.clear();
  }
};

static ProfilerMessageServer message_server;
bool read_and_perform_message(uint16_t type, uint32_t len, IGenLoad &lcb, IGenSave &scb)
{
  return message_server.perform(type, len, lcb, scb);
}
void add_message(uint16_t t, read_and_apply_message_cb_t cb) { message_server.addMessage(t, cb); }

} // namespace da_profiler
