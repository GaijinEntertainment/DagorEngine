// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "websocket_common.h"

#include <perfMon/dag_cpuFreq.h>

namespace websocket
{

struct IoSettings
{
  unsigned maxMessageSize = 0;
  bool verifyPeer = true;
  bool verifyHost = true;
  int connectTimeoutMs = 0;
  int closeTimeoutMs = 0;
};

struct ConnectRequest
{
  eastl::string uri;
  eastl::string ipResolveEntry;
  Headers headers;
};

// constraint: moving an empty MoveOnlyFunction reads an uninitialized field, so empty callbacks stay null pointers
using ConnectCbPtr = eastl::unique_ptr<ConnectCb>;

struct IoEvent
{ //-V802
  enum class Type : uint8_t
  {
    Connect,
    Message,
    Close,
    Detached
  };

  eastl::string ip;
  Error error;
  Payload payload;
  ConnectCbPtr connectCb;
  ConnectStatus connectStatus = ConnectStatus::Ok;
  CloseStatusInt closeStatus = 0;
  Type type = Type::Message;
  bool isText = false;
};

class IoSlot;

// One connection attempt and its WebSocket session. All methods run on the I/O thread.
class IoConnection
{
public:
  IoConnection(const IoSettings &io_settings, IoSlot &io_slot) : settings(io_settings), slot(io_slot) {}
  IoConnection(const IoConnection &) = delete;
  IoConnection &operator=(const IoConnection &) = delete;
  virtual ~IoConnection() = default;

  // Adds the easy handle to `multi`; on failure emits the connect failure and finishes
  virtual void start(CURLM *multi, const ConnectRequest &request) = 0;
  virtual void onTransferDone(CURLcode code) = 0;
  virtual void send(Payload &&payload, bool is_text) = 0;
  virtual void close(CloseStatus status) = 0;
  // Called when the wait socket is ready, a deadline passed, a command arrived or hasPendingWork() is true
  virtual void perform() = 0;
  // CURL_SOCKET_BAD while the multi watches the transfer itself
  virtual curl_socket_t getWaitSocket(short &events) const = 0;
  virtual bool getDeadline(int &deadline_ms) const = 0;
  virtual bool hasPendingWork() const = 0;

  bool isFinished() const { return finished; }

  // Owned by the I/O loop
  bool performRequested = false;
  short readyEvents = 0;

protected:
  void emitConnected(const char *peer_ip);
  void emitConnectFailed(Error &&error);
  void emitMessage(Payload &&payload, bool is_text);
  void emitClosed(CloseStatusInt status, Error &&error);

  const IoSettings &settings;

private:
  IoSlot &slot;
  bool finished = false;
};

using IoConnectionFactory = eastl::unique_ptr<IoConnection> (*)(const IoSettings &settings, IoSlot &slot);

eastl::unique_ptr<WebSocketClient> create_io_client(ClientConfig &&config, IoConnectionFactory factory);

inline int ms_since(int time_ms) { return int(unsigned(get_time_msec()) - unsigned(time_ms)); }

// For tests
struct IoStats
{
  unsigned ioWakeups = 0; // returns from the I/O thread wait since process start
  unsigned ioClients = 0;
  bool ioThreadRunning = false;
  bool ioWakeSocket = false; // the I/O thread wakes through its own socket, not curl_multi_wakeup
};
IoStats get_io_stats();

// Skips the curl_multi_wakeup probe of the next I/O manager, which the first client creates
void force_wake_socket_for_tests(bool force);
// Drops every wakeup, as if each one failed to send
void drop_wakeups_for_tests(bool drop);

} // namespace websocket
