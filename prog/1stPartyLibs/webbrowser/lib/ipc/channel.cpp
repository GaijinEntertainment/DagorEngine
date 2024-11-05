// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "channel.h"

#include <assert.h>
#include <windows.h>

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

namespace ipc
{

static const char host_addr[] = "127.0.0.1";

Channel::Channel(MessageHandler& h, webbrowser::ILogger& l)
  : webbrowser::EnableLogging(l), handle(h), sent(0)
{
  /* VOID */
}


Channel::~Channel()
{
  DBG("%s: sent %d", __FUNCTION__, this->sent);
  this->shutdown();
}


bool Channel::connect(unsigned short port)
{
  DBG("Channel::connect");
  return this->sock.connect(host_addr, port, true);
}


void Channel::shutdown()
{
  DBG("Channel::shutdown");
  this->sock.close();
}


bool Channel::send(const uint8_t *buf, uint32_t len)
{
  if (this->sock.send((const uint8_t*)&len, sizeof(len)) && len == this->sock.send(buf, len))
  {
    this->sent++;
    return true;
  }

  if (!this->sock.isValid())
  {
    WRN("Channel::send(): socket died, err=%d", this->sock.lastError());
    this->handle.onChannelError();
  }

  return false;
}


size_t Channel::process()
{
  size_t processed = 0;
  for (Message::size_type len = 0; this->sock.isValid() && this->sock.recv((uint8_t*)&len, sizeof(len)); len = 0)
  {
    Message buf(len);
    size_t r = 0;
    while (this->sock.isValid() && r < buf.size)
    {
      size_t l = this->sock.recv(buf.data + r, buf.size - r);
      if (this->sock.isValid())
        r += l;
    }

    if (this->sock.isValid() && r == buf.size && this->handle(buf))
    {
      processed++;
      continue;
    }

    if (!this->sock.isValid())
      WRN("Channel::process(): socket died, err=%d", this->sock.lastError());
    else // Failed to process message
      WRN("Channel::process(): aborting due to corrupt data in the channel, last err=%d",
          this->sock.lastError());

    this->handle.onChannelError();
  }

  return processed;
}


} // namespace ipc
