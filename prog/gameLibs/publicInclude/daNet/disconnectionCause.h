//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>

#define DANET_DEFINE_DISCONNECTION_CAUSES                                                                    \
  DC(DC_CONNECTION_LOST)            /* Connection was lost, most likely timeouted (abnormal disconnect) */   \
  DC(DC_CONNECTION_CLOSED)          /* Normal disconnect (remote system closed/destroyed it's connection) */ \
  DC(DC_CONNECTTION_ATTEMPT_FAILED) /* Connect attempt to remote system (server) failed */                   \
  DC(DC_CONNECTION_STOPPED)         /* Connection was stopped (typically - remote system fatal'ed) */        \
  DC(DC_NET_PROTO_MISMATCH)         /* Incompatible network protocol */                                      \
  DC(DC_SERVER_FULL)                /* Server is full (no free incoming connections) */                      \
  DC(DC_KICK_GENERIC)               /* User was kicked (generic) */                                          \
  DC(DC_KICK_INACTIVITY)            /* User was kicked due to inactivity */                                  \
  DC(DC_KICK_ANTICHEAT)             /* User was kicked by anticheat cause */                                 \
  DC(DC_KICK_FRIENDLY_FIRE)         /* User was kicked for friendly fire */                                  \
  DC(DC_KICK_VOTE)                  /* User was kicked by voting */

enum DisconnectionCause : uint8_t
{
#define DC(x) x,
  DANET_DEFINE_DISCONNECTION_CAUSES
#undef DC
    DC_NUM
};

const char *describe_disconnection_cause(DisconnectionCause cause);
