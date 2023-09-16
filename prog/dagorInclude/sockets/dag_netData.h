//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
