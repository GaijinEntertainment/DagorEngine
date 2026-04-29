// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <perfMon/dag_cpuFreq.h>

#include <dagCrypto/rand.h>
#include <websocket/websocket.h>


namespace websocketjsonrpc::abstraction
{

static WebsocketClientFactoryPtr custom_websocket_client_factory;
static TimerPtr custom_timer;


void set_custom_websocket_client_factory(const WebsocketClientFactoryPtr &factory) { custom_websocket_client_factory = factory; }


void set_custom_timer_implementation(const TimerPtr &timer) { custom_timer = timer; }


WebSocketClientPtr create_websocket_client(websocket::ClientConfig &&config)
{
  if (custom_websocket_client_factory)
    return custom_websocket_client_factory->createClient(eastl::move(config));

  return websocket::create_client(eastl::move(config));
}


TickCount get_current_tick_count()
{
  if (custom_timer)
    return custom_timer->getCurrentTickCount();

  return ref_time_ticks();
}


TickCount get_ticks_per_second()
{
  if (custom_timer)
    return custom_timer->getTicksPerSecond();

  return ref_ticks_frequency();
}


uint8_t generate_random_uint8()
{
  uint8_t value = 0;
  crypto::rand_bytes(&value, sizeof(value));
  return value;
}


uint16_t generate_random_uint16()
{
  uint16_t value = 0;
  crypto::rand_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(value));
  return value;
}


uint32_t generate_random_uint32()
{
  uint32_t value = 0;
  crypto::rand_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(value));
  return value;
}


uint64_t generate_random_uint64()
{
  uint64_t value = 0;
  crypto::rand_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(value));
  return value;
}


} // namespace websocketjsonrpc::abstraction
