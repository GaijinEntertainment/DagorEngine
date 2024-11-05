//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>

// Server information
struct NetServerInfo
{
  String userFriendlyName;
  String address;
  int clientsNum;
  int state;
};
