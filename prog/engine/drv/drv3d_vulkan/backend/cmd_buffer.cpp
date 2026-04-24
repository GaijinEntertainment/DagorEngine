// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cmd_buffer.h"
#include "cmds.h"
#include "backend/context.h"
#include "backend.h"
#include "stacked_profile_events.h"
#include "util/fault_report.h"

using namespace drv3d_vulkan;

namespace
{

enum CommandSubtype
{
  silent,
  measured,
};

} // namespace

#define DUMP_DEFAULT(type) \
  TSPEC void FaultReportDump::dumpCmd(const type &) {}
#define DUMP_CUSTOM(type)
#define CMD(type, subtype, dump_mode) dump_mode(type);
#include "cmd_list.inc.h"
#undef CMD
#undef DUMP_DEFAULT
#undef DUMP_CUSTOM

#if DAGOR_DBGLEVEL > 0
#define CMD(type, subtype, dump_mode)                                                     \
  template <>                                                                             \
  void FaultReportDump::execCmd(const type &v)                                            \
  {                                                                                       \
    currentCommand = addTagged(GlobalTag::TAG_CMD, (uint64_t)cmdPtr, String(#type));      \
    addRef(currentCommand, GlobalTag::TAG_WORK_ITEM, workItemId);                         \
    if (Globals::cfg.bits.recordCommandCaller)                                            \
    {                                                                                     \
      addRef(currentCommand, FaultReportDump::GlobalTag::TAG_CALLER_HASH, *callerHashes); \
      ++callerHashes;                                                                     \
    }                                                                                     \
    dumpCmd(v);                                                                           \
  }
#else
#define CMD(type, subtype, dump_mode)                                                \
  template <>                                                                        \
  void FaultReportDump::execCmd(const type &v)                                       \
  {                                                                                  \
    currentCommand = addTagged(GlobalTag::TAG_CMD, (uint64_t)cmdPtr, String(#type)); \
    addRef(currentCommand, GlobalTag::TAG_WORK_ITEM, workItemId);                    \
    dumpCmd(v);                                                                      \
  }
#endif
#include "cmd_list.inc.h"
#undef CMD

// only for dev builds with profiler
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
#define PROFILE_MARKER(type)                                                                                       \
  static ::da_profiler::desc_id_t cmdMarker = add_description(nullptr, 0, ::da_profiler::Normal, "vulkan_" #type); \
  Backend::profilerStack.pushChained(cmdMarker);
#else
#define PROFILE_MARKER(type)
#endif

template <typename T>
void BECmdBuffer::exec(T &ctx)
{
  execPtr = mem.data();
  while (execPtr < mem.end())
  {
    CmdID id = *((CmdID *)execPtr);
    execPtr += sizeof(CmdID);
    ctx.beginCmd((void *)execPtr);
    switch (id)
    {
//-V:CMD:501
#define CMD(type, subtype, dump_mode)                        \
  case cmd_get__ID<type>():                                  \
    if (CommandSubtype::subtype == CommandSubtype::measured) \
    {                                                        \
      PROFILE_MARKER(type);                                  \
    }                                                        \
    {                                                        \
      type val;                                              \
      memcpy((void *)&val, execPtr, sizeof(type));           \
      execPtr += sizeof(type);                               \
      ctx.execCmd(val);                                      \
    }                                                        \
    break
#include "cmd_list.inc.h"
#undef CMD
      default: G_ASSERTF(0, "vulkan: unhandled be command %u", id); break;
    }
    ctx.endCmd();
  }
}

void BECmdBuffer::clear() { mem.clear(); }

template void BECmdBuffer::exec(BEContext &ctx);
template void BECmdBuffer::exec(FaultReportDump &ctx);
