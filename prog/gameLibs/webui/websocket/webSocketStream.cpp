// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/websocket/webSocketStream.h>

#include <time.h>
#include <mongoose/mongoose.h>

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <memory/dag_framemem.h>
#include <util/dag_simpleString.h>
#include <util/dag_console.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_logSys.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tab.h>

#if _TARGET_PC_WIN
#include <Ws2tcpip.h>
#endif

namespace websocket
{
WinCritSec crit_sec;

class WebSocketStream : public DaThread
{
  bool readyForRead;
  int currentWriteBuffer;
  int currentReadBuffer;

  typedef Tab<uint8_t> RawBytes;

  struct Message
  {
    bool isBinary;
    uint32_t start;
    size_t length;
  };

  struct Buffer
  {
    RawBytes data;
    Tab<Message> messages;
  };

  struct InputMessage
  {
    RawBytes data;
    mg_connection *connection;
  };

  struct Connection
  {
    bool needClose;
    mg_connection *connection;
  };

  carray<Buffer, 2> buffers;
  mg_context *ctx;

  Tab<Connection> connections;
  Tab<InputMessage> inputMessages;

  Tab<MessageListener *> listeners;

  time_t nextPing;

public:
  int port = -1;

  WebSocketStream() :
    DaThread("WebSocketStream", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK),
    currentWriteBuffer(0),
    currentReadBuffer(-1),
    readyForRead(true),
    ctx(NULL),
    nextPing(0)
  {
    for (int i = 0; i < buffers.size(); ++i)
    {
      dag::set_allocator(buffers[i].data, midmem_ptr());
      dag::set_allocator(buffers[i].messages, midmem_ptr());
    }
  }

  ~WebSocketStream()
  {
    if (ctx)
    {
      // WinAutoLock lock(crit_sec);
      mg_stop(ctx);
      ctx = NULL;
    }
  }

  void doExecute()
  {
    if (!ctx)
      return;

    if (currentReadBuffer >= 0 && !connections.empty())
    {
      Buffer &buffer = buffers[currentReadBuffer];
      for (int i = 0; i < buffer.messages.size(); ++i)
      {
        const Message &message = buffer.messages[i];
        for (int j = 0; j < connections.size(); ++j)
          mg_websocket_write(connections[j].connection, message.isBinary ? WEBSOCKET_OPCODE_BINARY : WEBSOCKET_OPCODE_TEXT,
            (const char *)&buffer.data[message.start], message.length);
      }

      buffer.data.clear();
      buffer.messages.clear();

      currentReadBuffer = -1;
      readyForRead = true;
    }

    mg_process_requests(ctx);
  }

  virtual void execute()
  {
    if (!ctx)
    {
      G_ASSERT_RETURN(port > 0, );

      String portStr(0, "%d", port);
      const char *options[] = {"listening_ports", portStr.str(), "num_threads", "2", NULL};

      mg_callbacks callbacks;
      memset(&callbacks, 0, sizeof(callbacks));
      callbacks.websocket_ready = websocket_ready_hadler;
      callbacks.websocket_data = websocket_data_hadler;

      ctx = mg_start(&callbacks, this, options);
      if (!ctx)
        DEBUG_CTX("[WEBSOCKET] Failed to start websocket server");
      else
        DEBUG_CTX("[WEBSOCKET] The server has been started at port %d", port);
    }

    while (!isThreadTerminating())
    {
      doExecute();
      sleep_msec(10);
    }
  }

  void sendMessage(const uint8_t *message, size_t length, bool is_binary = false)
  {
    G_ASSERT(currentWriteBuffer != currentReadBuffer);

    Buffer &buffer = buffers[currentWriteBuffer];

    uint32_t start = buffer.data.size();
    append_items(buffer.data, length, message);

    Message &msg = buffer.messages.push_back();
    msg.isBinary = is_binary;
    msg.start = start;
    msg.length = length;
  }

  void addMessageListener(MessageListener *listener) { listeners.push_back(listener); }

  void removeMessageListener(MessageListener *listener)
  {
    for (int i = 0; i < listeners.size(); ++i)
      if (listeners[i] == listener)
      {
        listeners[i] = nullptr;
        break;
      }
  }

  void updateListeners()
  {
    {
      WinAutoLock lock(crit_sec);
      if (connections.empty())
        return;
    }

    for (int i = 0; i < listeners.size(); ++i)
      if (listeners[i])
        listeners[i]->update();
  }

  void update()
  {
    if (readyForRead && !buffers[currentWriteBuffer].messages.empty())
    {
      readyForRead = false;
      currentReadBuffer = currentWriteBuffer;
      currentWriteBuffer = (currentWriteBuffer + 1) % buffers.size();
    }

    {
      WinAutoLock lock(crit_sec);
      for (int i = 0; i < inputMessages.size(); ++i)
      {
        const InputMessage &input = inputMessages[i];
        DataBlock blk;
        if (!blk.loadText((const char *)&input.data[0], input.data.size()))
          continue;

        for (int j = listeners.size() - 1; j >= 0; --j)
          if (listeners[j])
          {
            if (blk.paramExists("cmd"))
              listeners[j]->processCommand(blk.getStr("cmd"), blk);

            listeners[j]->onMessage(blk);
          }
          else
            erase_items(listeners, j, 1);
      }
      inputMessages.clear();
    }

    time_t currentTime = ::time(NULL);
    if (currentTime > nextPing)
    {
      WinAutoLock lock(crit_sec);

      nextPing = currentTime + 10;

      for (int i = connections.size() - 1; i >= 0; --i)
      {
        if (connections[i].needClose)
        {
          // mg_close_connection(connections[i].connection);
          erase_items(connections, i, 1);
        }
      }

      for (int i = 0; i < connections.size(); ++i)
      {
        connections[i].needClose = true;
        mg_websocket_write(connections[i].connection, WEBSOCKET_OPCODE_PING, "__ping__", 8);
      }
    }

    updateListeners();
  }

  void addConnection(mg_connection *conn)
  {
    WinAutoLock lock(crit_sec);

    for (int i = 0; i < connections.size(); ++i)
      if (connections[i].connection == conn)
        return;

    Connection &c = connections.push_back();
    c.connection = conn;
    c.needClose = false;
  }

  void removeConnection(mg_connection *conn)
  {
    WinAutoLock lock(crit_sec);

    for (int i = connections.size() - 1; i >= 0; --i)
    {
      if (connections[i].connection == conn)
      {
        // mg_close_connection(conn);
        erase_items(connections, i, 1);
        break;
      }
    }
  }

  void addMessage(mg_connection *conn, RawBytes &message)
  {
    WinAutoLock lock(crit_sec);

    InputMessage &input = inputMessages.push_back();
    input.connection = conn;
    input.data = eastl::move(message);
  }

  void pong(mg_connection *conn)
  {
    WinAutoLock lock(crit_sec);

    for (int i = 0; i < connections.size(); ++i)
    {
      if (connections[i].connection == conn)
      {
        connections[i].needClose = false;
        mg_websocket_write(connections[i].connection, WEBSOCKET_OPCODE_PONG, "__ping__", 8);
        break;
      }
    }
  }

  static void websocket_ready_hadler(mg_connection *conn)
  {
    WebSocketStream *self = (WebSocketStream *)mg_get_request_info(conn)->user_data;
    if (self)
      self->addConnection(conn);
  }

  static void websocket_read(mg_connection *conn, RawBytes &message)
  {
    RawBytes body;
    for (;;)
    {
      uint8_t tmpBuffer[4096];
      const int bytesRead = mg_read(conn, tmpBuffer, sizeof(tmpBuffer));
      if (bytesRead <= 0)
        break;

      append_items(body, bytesRead, tmpBuffer);
    }

    if (body.size() < 6)
      return;

    const int length = body[1] & 127;
    const int maskIndex = length == 126 ? 4 : length == 127 ? 10 : 2;
    const int bodyStart = maskIndex + 4;
    for (int i = bodyStart, j = 0; i < body.size(); ++i, ++j)
      body[i] ^= body[maskIndex + (j % 4)];

    if (bodyStart < body.size())
      message = make_span_const(&body[bodyStart], body.size() - bodyStart);
  }

  static int websocket_data_hadler(mg_connection *conn)
  {
    WebSocketStream *self = (WebSocketStream *)mg_get_request_info(conn)->user_data;
    if (!self)
      return 0;

    RawBytes message;
    websocket_read(conn, message);

    if (message.size() == 2 && message[0] == 0x03 && message[1] == 0xe9)
    {
      self->removeConnection(conn);
      return 0;
    }

    if (message[0] == 0x03)
    {
      self->removeConnection(conn);
      return 0;
    }

    if (message.size() == 8 && ::strncmp("__ping__", (const char *)&message[0], 8) == 0)
    {
      self->pong(conn);
      return 1;
    }

    if (!message.empty())
    {
      self->addMessage(conn, message);
      return 1;
    }

    self->removeConnection(conn);

    return 0;
  }
};

static InitOnDemand<WebSocketStream> websocket_stream;

void MessageListener::addCommand(const char *name, const Method &method)
{
  commands.insert(CommandMap::value_type(SimpleString(name), method));
}

bool MessageListener::processCommand(const char *name, const DataBlock &blk)
{
  CommandMap::iterator it = commands.find_as(name);
  if (it == commands.end())
    return false;

  Method method = it->second;
  (this->*method)(blk);

  return true;
}

class DefaultMessageListener : public MessageListener
{
public:
  bool needTailLog;
  bool needWatchLog;
  int lastSentLogPosition;

  DefaultMessageListener() : lastSentLogPosition(0), needTailLog(true), needWatchLog(false)
  {
    addCommand("console", (Method)&DefaultMessageListener::runConsole);
    addCommand("needWatchLog", (Method)&DefaultMessageListener::enableWatchLog);
    addCommand("resendLog", (Method)&DefaultMessageListener::resendLog);
  }

  void enableWatchLog(const DataBlock &blk) { needWatchLog = blk.getBool("needWatchLog", false); }

  void resendLog(const DataBlock & /*blk*/) { needTailLog = true; }

  void runConsole(const DataBlock &blk)
  {
    String cmd(framemem_ptr());

    cmd += blk.getStr("command");
    int ac = 0;
    for (;; ++ac)
    {
      String tmp(256, "arg%d", ac);
      if (!blk.paramExists(tmp.str()))
        break;

      cmd += " ";
      cmd += blk.getStr(tmp);
    }

    console::command(cmd.str());
  }

  void watchLog()
  {
    debug_flush(false);

    file_ptr_t ld = df_open(get_log_filename(), DF_READ);
    if (!ld)
      return;

    int file_size = df_length(ld);
    if (file_size <= 0)
    {
      df_close(ld);
      return;
    }

    if (df_seek_to(ld, needTailLog ? file_size - 1024 * 16 : lastSentLogPosition) < 0)
    {
      df_close(ld);
      return;
    }

    char lineBuffer[1024 * 16];

    for (;;)
    {
      const char *line = df_gets(lineBuffer, sizeof(lineBuffer), ld);
      if (!line)
        break;

      send_buffer(line, (int)::strlen(line));

      if (!needTailLog)
        break;
    }

    needTailLog = false;
    lastSentLogPosition = df_tell(ld);

    df_close(ld);
  }

  virtual void update()
  {
    if (needWatchLog)
      watchLog();
  }
};

static DefaultMessageListener websocket_listener;
static bool started = false;

int get_port() { return websocket_stream ? websocket_stream->port : -1; }

bool is_enabled() { return ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("enableWebSocketStream", false); }

bool start(int port)
{
  if (!is_enabled())
  {
    debug("[WEBSOCKET]: Disabled");
    return false;
  }

  if (started)
    return true;

  websocket_stream.demandInit();

  websocket_stream->port = port;

  if (websocket_stream->start())
  {
    websocket_stream->addMessageListener(&websocket_listener);
    started = true;
    return true;
  }

  return false;
}

void stop()
{
  if (!websocket_stream)
    return;

  websocket_stream->terminate(true, 100);
  websocket_stream.demandDestroy();
}

void update()
{
  if (websocket_stream)
    websocket_stream->update();
}

void send_log(const char *message)
{
  if (!websocket_stream)
    return;

  send_buffer(message, (int)::strlen(message));
}

void send_message(const DataBlock &blk)
{
  if (!websocket_stream)
    return;

  BsonStream bson;
  bson.add(blk);

  send_binary(bson.closeAndGetData());
}

void send_buffer(const char *buffer, int length)
{
  if (websocket_stream)
    websocket_stream->sendMessage((const uint8_t *)buffer, length);
}

void send_binary(dag::ConstSpan<uint8_t> buffer)
{
  if (websocket_stream)
    websocket_stream->sendMessage(buffer.data(), buffer.size(), true);
}

void send_binary(BsonStream &bson) { send_binary(bson.closeAndGetData()); }

void send_binary(const uint8_t *buffer, int length)
{
  if (websocket_stream)
    websocket_stream->sendMessage(buffer, length, true);
}

void add_message_listener(MessageListener *listener)
{
  websocket_stream.demandInit();
  websocket_stream->addMessageListener(listener);
}

void remove_message_listener(MessageListener *listener)
{
  if (websocket_stream)
    websocket_stream->removeMessageListener(listener);
}
} // namespace websocket