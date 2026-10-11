//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/net/msgDecl.h>

ECS_NET_DECL_TIMED_MSG(TimeSyncRequest, int /*clientTime*/);                      // client's request
ECS_NET_DECL_TIMED_MSG(TimeSyncResponse, int /*clientTime*/, int /*serverTime*/); // server's response

#define TIME_SYNC_INTERVAL_MS 1500

class ITimeManager
{
public:
  virtual ~ITimeManager() {}

  virtual double getSeconds() const = 0;  // synced from server time in seconds
  virtual int getAsyncMillis() const = 0; // not synced time in milliseconds

  virtual double advance(float dt, float &rt_dt) = 0; // returns current time in seconds and sets rt_dt to amount of time has advanced

  virtual void onReceiveTimeSyncResponse(const TimeSyncResponse *) { G_ASSERT(0); }
};

float get_sync_time();
double get_sync_time_d();
int get_sync_millis();
int get_async_millis();
void on_time_sync_response_received(const net::IMessage *msg);

ITimeManager *create_server_time();
ITimeManager *create_client_time();
ITimeManager *create_replay_time(double start_time = 0.0);
ITimeManager *create_accum_time();

// Unsatisfied link-time deps. Users of synctime lib should provide link time definitions for these.
extern const uint8_t TIMESYNC_NET_CHANNEL;
extern void timesync_push_rtt(uint32_t rtt);
