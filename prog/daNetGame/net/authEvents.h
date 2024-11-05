// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

#define NET_AUTH_ECS_EVENTS                                                                             \
  NET_AUTH_ECS_EVENT(NetAuthEventOnInit, const char * /*game*/, const char * /*distr*/, int /*appId*/)  \
  NET_AUTH_ECS_EVENT(NetAuthEventOnTerm)                                                                \
  NET_AUTH_ECS_EVENT(NetAuthEventOnUpdate)                                                              \
  NET_AUTH_ECS_EVENT(NetAuthGetRemoteVersionAsyncEvent, const char * /*project*/, const char * /*tag*/) \
  NET_AUTH_ECS_EVENT(NetAuthGetCountryCodeEvent, eastl::string * /*out_country_code*/)                  \
  NET_AUTH_ECS_EVENT(NetAuthGetUserTokenEvent, eastl::string * /*out_token*/)                           \
  NET_AUTH_ECS_EVENT(NetAuthGetOpaqueSessionPtrEvent, void ** /*out_session_ptr*/)

#define NET_AUTH_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
NET_AUTH_ECS_EVENTS
#undef NET_AUTH_ECS_EVENT
