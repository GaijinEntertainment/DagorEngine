//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>

#include <util/dag_stdint.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>


namespace websocket
{
struct ClientConfig;
class WebSocketClient;
} // namespace websocket


namespace websocketjsonrpc::abstraction
{

using WebSocketClientPtr = eastl::unique_ptr<websocket::WebSocketClient>;


class WebsocketClientFactory
{
public:
  virtual ~WebsocketClientFactory() = default;
  virtual WebSocketClientPtr createClient(websocket::ClientConfig &&config) = 0;
};


using WebsocketClientFactoryPtr = eastl::shared_ptr<WebsocketClientFactory>;


class Timer
{
public:
  virtual ~Timer() = default;

  virtual TickCount getCurrentTickCount() const = 0;
  virtual TickCount getTicksPerSecond() const = 0; // must be constant during the object lifetime
};


using TimerPtr = eastl::shared_ptr<Timer>;


WebSocketClientPtr create_websocket_client(websocket::ClientConfig &&config);

TickCount get_current_tick_count();

TickCount get_ticks_per_second();

uint8_t generate_random_uint8();
uint16_t generate_random_uint16();
uint32_t generate_random_uint32();
uint64_t generate_random_uint64();

// This function is for testing purposes only; nullptr value means using default implementation
void set_custom_websocket_client_factory(const WebsocketClientFactoryPtr &factory);

// This function is for testing purposes only; nullptr value means using default implementation
void set_custom_timer_implementation(const TimerPtr &timer);


} // namespace websocketjsonrpc::abstraction
