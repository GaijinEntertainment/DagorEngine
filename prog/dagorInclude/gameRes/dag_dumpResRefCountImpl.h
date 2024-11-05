//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#define IMPLEMENT_DUMP_RESOURCES_REF_COUNT(DATA, RESID, REFC)         \
  virtual void dumpResourcesRefCount() override                       \
  {                                                                   \
    String name;                                                      \
    debug("%s: %d active resources", getResClassName(), DATA.size()); \
    for (unsigned int resNo = 0; resNo < DATA.size(); resNo++)        \
      if (DATA[resNo].REFC)                                           \
      {                                                               \
        get_game_resource_name(DATA[resNo].RESID, name);              \
        debug("    '%s', refCount=%d", name, DATA[resNo].REFC);       \
      }                                                               \
  }
