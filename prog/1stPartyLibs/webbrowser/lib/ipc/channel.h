// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "socket.h"

#include "../log.h"


namespace ipc
{

class Channel : public webbrowser::EnableLogging
{
public:
  struct Message
  {
    typedef uint32_t size_type;

    Message(size_type sz) : size(sz)
    {
      if (this->size > sizeof(staticBuf))
      {
        this->data = new uint8_t[this->size];
        memset(this->data, 0xFF, this->size);
      }
      else
      {
        this->data = staticBuf;
        memset(this->staticBuf, 0xFF, sizeof(staticBuf));
      }
    }

    ~Message()
    {
      if (this->data != this->staticBuf)
        delete [] this->data;
    }

    uint8_t *data = nullptr;
    size_type size = 0;

  private:
    uint8_t staticBuf[1024];
  }; // struct Message


  struct MessageHandler
  {
    virtual bool operator()(const Message&) = 0;
    virtual void onChannelError() = 0;
  };

public:
  Channel(MessageHandler& h, webbrowser::ILogger& l);
  virtual ~Channel();

public:
  bool connect(unsigned short);
  void shutdown();

  bool isConnected() { return this->sock.isValid(); }
  Socket::error_t error() { return this->sock.lastError(); }
  bool send(const uint8_t *buf, uint32_t len);
  size_t process(); // returns number of messages processed

  Socket& socket() { return this->sock; }

private:
  MessageHandler& handle;
  Socket sock;
  int sent;
}; // class Channel

} // namespace ipc
