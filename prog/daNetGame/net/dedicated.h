// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <matching/types.h>
#include <daECS/net/msgSink.h>
#include "net/plosscalc.h"

class String;

namespace net
{
class INetDriver;
class IConnection;
class INetworkObserver;
} // namespace net

namespace dedicated
{

#if _TARGET_PC
bool is_dedicated();
#else
inline constexpr bool is_dedicated() { return false; }
#endif
void update();
void shutdown();
net::msg_handler_t get_clientinfo_msg_handler();
net::msg_handler_t get_sync_vroms_done_msg_handler();
String setup_log();
bool init_statsd(const char *circuit_name); // return false if stubbed
void statsd_report_ctrl_ploss(ControlsPlossCalc &ploss_calc, uint16_t &last_reported_ctrl_seq, uint16_t new_ctrl_seq);
net::INetDriver *create_net_driver();
uint16_t get_binded_port();
const char *get_host_url(eastl::string &hurl, int &it);
net::INetworkObserver *create_server_net_observer(void *buf, size_t bufsz);
int get_server_tick_time_us();
bool is_dynamic_tickrate_enabled();

extern size_t number_of_tickrate_changes;

} // namespace dedicated
