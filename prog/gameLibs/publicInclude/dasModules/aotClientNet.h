//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daNet/daNetEchoManager.h>

DAS_BIND_ENUM_CAST(danet::EchoResponse::Result);
DAS_BASE_BIND_ENUM(danet::EchoResponse::Result, EchoResponseResult, SUCCESS, UNRESOLVED, TIMEOUT)

uint32_t get_current_server_route_id();
const char *get_server_route_host(uint32_t route_id);
uint32_t get_server_route_count();
void switch_server_route(uint32_t route_id);
void send_echo_msg(uint32_t route_id);
