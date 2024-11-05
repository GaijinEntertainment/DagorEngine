// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include <util/dag_string.h>
#include <daECS/net/network.h>
#include <daECS/net/network.h>
#include "net/netPrivate.h"
#include <ecs/core/entityManager.h>
#include "net/time.h"
#include <daECS/net/netEvents.h>


class SqModules;


namespace dedicated
{

#if _TARGET_PC
bool is_dedicated() { return false; }
#endif
void init() {}
void update() {}
void shutdown() {}
String setup_log() { return String(); }
bool init_statsd(const char *) { return false; }
void statsd_report_ctrl_ploss(ControlsPlossCalc &, uint16_t &, uint16_t) {}
net::INetDriver *create_net_driver() { return nullptr; }
uint16_t get_binded_port() { return 0; }
net::msg_handler_t get_clientinfo_msg_handler() { return NULL; }
net::msg_handler_t get_sync_vroms_done_msg_handler() { return NULL; }
net::INetworkObserver *create_server_net_observer(void *, size_t) { return NULL; }
int get_server_tick_time_us() { return 0; }
bool is_dynamic_tickrate_enabled() { return false; };

size_t number_of_tickrate_changes = 0;

} // namespace dedicated

void pull_dedicated_das() {}

void on_sync_vroms_done_msg(const net::IMessage *) {}
