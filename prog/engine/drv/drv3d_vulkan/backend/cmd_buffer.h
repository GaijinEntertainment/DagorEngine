// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tab.h>

namespace drv3d_vulkan
{

#define CMD(type, subtype, dump_mode) struct type;
#include "cmd_list.inc.h"
#undef CMD

struct BECmdBuffer
{
  Tab<uint8_t> mem;
  uint8_t *execPtr = nullptr;

  template <typename T>
  void push(const T &val)
  {
    uint8_t *ptr = mem.append_default(sizeof(T) + sizeof(CmdID));
    *((CmdID *)ptr) = cmd_get__ID<T>();
    ptr += sizeof(CmdID);
    memcpy(ptr, &val, sizeof(T));
  }

  template <typename T>
  bool isNextCommandSame(const T &)
  {
    const uint8_t *ptr = execPtr;
    ptr += sizeof(T);
    if (ptr >= mem.end())
      return false;
    CmdID id = *((const CmdID *)ptr);
    return id == cmd_get__ID<T>();
  }

  template <typename T>
  void exec(T &ctx);

  void clear();


  static constexpr size_t COUNTER_BASE = (__COUNTER__);

private:
  typedef uint8_t CmdID;

  template <typename T>
  static constexpr CmdID cmd_get__ID();
};

#define AUTO_ID (__COUNTER__ - COUNTER_BASE)
#define CMD(type, subtype, dump_mode)                           \
  template <>                                                   \
  constexpr BECmdBuffer::CmdID BECmdBuffer::cmd_get__ID<type>() \
  {                                                             \
    return AUTO_ID;                                             \
  }
#include "cmd_list.inc.h"
#undef CMD
#undef AUTO_ID

} // namespace drv3d_vulkan
